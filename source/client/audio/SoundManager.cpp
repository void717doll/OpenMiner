#include <algorithm>
#include <cmath>
#include <cstring>

#include "common/core/Debug.hpp"
#include "common/core/Exception.hpp"
#include "common/core/XMLFile.hpp"
#include "common/world/BlockState.hpp"
#include "common/world/Block.hpp"
#include "client/audio/SoundManager.hpp"

SoundManager::~SoundManager() {
	if (m_device) SDL_CloseAudioDevice(m_device);
	if (m_audioInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
	if (s_instance == this) s_instance = nullptr;
}

bool SoundManager::init(const std::string &configFile) {
	s_instance = this;
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		logWarning() << "audio unavailable:" << SDL_GetError();
		return false;
	}
	m_audioInitialized = true;
	SDL_AudioSpec wanted{};
	wanted.freq = 44100;
	wanted.format = AUDIO_F32SYS;
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = &SoundManager::mix;
	wanted.userdata = this;
	m_device = SDL_OpenAudioDevice(nullptr, 0, &wanted, &m_format, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	if (!m_device) {
		logWarning() << "audio device unavailable:" << SDL_GetError();
		return false;
	}
	try {
		XMLFile config(configFile);
		for (auto *group = config.FirstChildElement("sounds").FirstChildElement("group").ToElement();
			group; group = group->NextSiblingElement("group")) {
			const char *name = group->Attribute("name");
			if (!name) continue;
			auto &clips = m_groups[name];
			for (auto *entry = group->FirstChildElement("clip"); entry; entry = entry->NextSiblingElement("clip")) {
				const char *path = entry->Attribute("path");
				Clip clip;
				if (path && loadClip(path, clip)) clips.push_back(std::move(clip));
			}
		}
	} catch (const Exception &error) {
		logWarning() << "unable to load sounds:" << error.what();
		SDL_CloseAudioDevice(m_device);
		m_device = 0;
		return false;
	}
	SDL_PauseAudioDevice(m_device, 0);
	return true;
}

bool SoundManager::loadClip(const std::string &path, Clip &clip) {
	SDL_AudioSpec source{};
	Uint8 *data = nullptr;
	Uint32 length = 0;
	if (!SDL_LoadWAV(path.c_str(), &source, &data, &length)) {
		logWarning() << "unable to load sound:" << path << SDL_GetError();
		return false;
	}
	SDL_AudioCVT conversion{};
	int result = SDL_BuildAudioCVT(&conversion, source.format, source.channels, source.freq,
		m_format.format, m_format.channels, m_format.freq);
	if (result < 0) { SDL_FreeWAV(data); return false; }
	std::vector<Uint8> buffer(std::size_t(length) * std::max(1, conversion.len_mult));
	std::memcpy(buffer.data(), data, length);
	SDL_FreeWAV(data);
	conversion.buf = buffer.data();
	conversion.len = int(length);
	if (conversion.needed && SDL_ConvertAudio(&conversion) < 0) return false;
	std::size_t bytes = conversion.needed ? std::size_t(conversion.len_cvt) : length;
	clip.samples.resize(bytes / sizeof(float));
	std::memcpy(clip.samples.data(), buffer.data(), clip.samples.size() * sizeof(float));
	return clip.samples.size() >= 2;
}

u32 SoundManager::random() {
	m_random = m_random * 1664525u + 1013904223u;
	return m_random;
}

bool SoundManager::startVoice(const std::string &group, float gain, float pitch, float pan) {
	if (!m_device || gain <= 0) return false;
	auto found = m_groups.find(group);
	if (found == m_groups.end() || found->second.empty()) return false;
	const Clip &clip = found->second[random() % found->second.size()];
	SDL_LockAudioDevice(m_device);
	if (m_paused) { SDL_UnlockAudioDevice(m_device); return false; }
	auto voice = std::find_if(m_voices.begin(), m_voices.end(), [](const Voice &v) { return !v.clip; });
	if (voice == m_voices.end())
		voice = std::min_element(m_voices.begin(), m_voices.end(), [](const Voice &a, const Voice &b) { return a.serial < b.serial; });
	*voice = Voice{&clip, 0, std::clamp(pitch, 0.25f, 4.f), gain * std::sqrt((1 - pan) * 0.5f),
		gain * std::sqrt((1 + pan) * 0.5f), ++m_serial};
	SDL_UnlockAudioDevice(m_device);
	return true;
}

bool SoundManager::play(const std::string &group, float gain, float pitch) {
	return startVoice(group, gain, pitch, 0);
}

bool SoundManager::playAt(const std::string &group, const Vector3d &position, float gain, float pitch) {
	Vector3d delta = position - m_listener;
	double distance = delta.length();
	double range = group == "item_drop_pickup" ? 16 :
		group == "tnt_explode" ? 64 : 32;
	if (distance > range) return false;
	float pan = distance > 0.01 ? float(std::clamp((delta.x * m_right.x + delta.y * m_right.y) / distance, -1.0, 1.0)) : 0;
	return startVoice(group, gain / float(1 + distance * distance / 64), pitch, pan);
}

void SoundManager::setListener(const Vector3d &position, const Vector3d &direction) {
	m_listener = position;
	double length = std::hypot(direction.x, direction.y);
	if (length > 0.001) m_right = {direction.y / length, -direction.x / length, 0};
}

void SoundManager::setVolume(float volume) {
	volume = std::clamp(volume, 0.f, 1.f);
	if (!m_device || m_volume == volume) return;
	SDL_LockAudioDevice(m_device);
	m_volume = volume;
	SDL_UnlockAudioDevice(m_device);
}

void SoundManager::setPaused(bool paused) {
	if (!m_device || m_paused == paused) return;
	SDL_LockAudioDevice(m_device);
	m_paused = paused;
	if (paused) m_voices = {};
	SDL_UnlockAudioDevice(m_device);
}

void SoundManager::mix(void *context, Uint8 *stream, int length) {
	auto &self = *static_cast<SoundManager *>(context);
	std::memset(stream, 0, std::size_t(length));
	if (self.m_paused) return;
	auto *output = reinterpret_cast<float *>(stream);
	std::size_t frames = std::size_t(length) / (2 * sizeof(float));
	// clips are decoded before playback; the audio callback only mixes fixed voices.
	for (auto &voice : self.m_voices) {
		if (!voice.clip) continue;
		const auto &samples = voice.clip->samples;
		std::size_t clipFrames = samples.size() / 2;
		for (std::size_t frame = 0; frame < frames; ++frame) {
			std::size_t index = std::size_t(voice.cursor);
			if (index >= clipFrames) { voice.clip = nullptr; break; }
			std::size_t next = std::min(index + 1, clipFrames - 1);
			float fraction = float(voice.cursor - double(index));
			float left = samples[index * 2] + (samples[next * 2] - samples[index * 2]) * fraction;
			float right = samples[index * 2 + 1] + (samples[next * 2 + 1] - samples[index * 2 + 1]) * fraction;
			output[frame * 2] += left * voice.left * self.m_volume;
			output[frame * 2 + 1] += right * voice.right * self.m_volume;
			voice.cursor += voice.pitch;
		}
	}
	for (std::size_t i = 0; i < frames * 2; ++i) output[i] = std::clamp(output[i], -1.f, 1.f);
}

void SoundManager::playBlock(const BlockState &state, BlockSound action, const Vector3d &position) {
	const auto &block = state.block();
	if (!block.id()) return;
	const auto &id = block.stringID();
	std::string material = "hard";
	if (id == "default:grass" || state.drawType() == BlockDrawType::XShape || state.drawType() == BlockDrawType::Leaves) material = "grass";
	else if (id == "default:gravel") material = "gravel";
	else if (id == "default:snow") material = "snow";
	else if (id == "default:ice") material = "ice";
	else if (state.drawType() == BlockDrawType::Glass) material = "glass";
	else if (block.hasGroup("group:om_material_wood") || id == "default:bookshelf") material = "wood";
	else if (block.hasGroup("group:om_material_dirt") || id == "default:clay" || id == "default:farmland") material = "dirt";
	else if (block.hasGroup("group:om_material_sand")) material = "sand";
	else if (id == "default:wool") material = "cloth";
	std::string group = "default_" + material + "_footstep";
	float gain = action == BlockSound::Step ? 0.2f : 1.f;
	if (action == BlockSound::Dig) {
		if (material == "ice") group = "default_ice_dig";
		else if (material == "hard" || material == "glass") group = "default_dig_cracky";
		else if (material == "wood") group = "default_dig_choppy";
		else if (material == "grass") group = "default_dig_snappy";
		else if (material == "gravel") group = "default_gravel_dig";
		else if (material == "dirt" || material == "sand") group = "default_dig_crumbly";
		gain = material == "hard" || material == "glass" ? 0.5f : 0.4f;
		if (material == "ice" || material == "gravel") gain = 0.35f;
	} else if (action == BlockSound::Dug && material == "ice") { group = "default_ice_dug"; gain = 0.35f; }
	else if (action == BlockSound::Dug && material == "glass") { group = "default_break_glass"; gain = 0.5f; }
	else if (action == BlockSound::Dug && material == "gravel") { group = "default_gravel_dug"; gain = 0.75f; }
	else if (action == BlockSound::Dug && material == "grass") gain = 0.425f;
	else if (action == BlockSound::Dug && material == "sand") gain = 0.15f;
	else if (action == BlockSound::Place) group = material == "hard" ? "default_place_node_hard" : "default_place_node";
	if (action == BlockSound::Step && material == "sand") gain = 0.05f;
	if (material == "cloth") {
		group = "mcl_sounds_cloth";
		gain = action == BlockSound::Step ? 0.2f : 0.8f;
	}
	if (material == "snow" && action != BlockSound::Place) {
		group = "pedology_snow_soft_footstep";
		gain = action == BlockSound::Step ? 0.4f : 1.f;
	}
	float pitch = 0.95f + float(random() % 101) / 1000.f;
	playAt(group, position, gain, pitch);
}
