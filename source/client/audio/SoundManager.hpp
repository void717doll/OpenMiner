#ifndef SOUNDMANAGER_HPP_
#define SOUNDMANAGER_HPP_

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL.h>

#include "common/core/Vector3.hpp"

class BlockState;

enum class BlockSound { Step, Dig, Dug, Place };

class SoundManager {
	public:
		SoundManager() = default;
		SoundManager(const SoundManager &) = delete;
		SoundManager &operator=(const SoundManager &) = delete;
		~SoundManager();
		bool init(const std::string &configFile);
		bool play(const std::string &group, float gain = 1, float pitch = 1);
		bool playAt(const std::string &group, const Vector3d &position, float gain = 1, float pitch = 1);
		void playBlock(const BlockState &state, BlockSound action, const Vector3d &position);
		void setListener(const Vector3d &position, const Vector3d &direction);
		void setVolume(float volume);
		void setPaused(bool paused);
		static SoundManager *instance() { return s_instance; }

	private:
		struct Clip { std::vector<float> samples; };
		struct Voice {
			const Clip *clip = nullptr;
			double cursor = 0;
			float pitch = 1, left = 1, right = 1;
			u64 serial = 0;
		};
		bool loadClip(const std::string &path, Clip &clip);
		bool startVoice(const std::string &group, float gain, float pitch, float pan);
		static void mix(void *context, Uint8 *stream, int length);
		u32 random();

		inline static SoundManager *s_instance = nullptr;
		SDL_AudioDeviceID m_device = 0;
		SDL_AudioSpec m_format{};
		bool m_audioInitialized = false;
		bool m_paused = false;
		float m_volume = 0.7f;
		std::unordered_map<std::string, std::vector<Clip>> m_groups;
		std::array<Voice, 24> m_voices{};
		Vector3d m_listener{0, 0, 0}, m_right{0, -1, 0};
		u32 m_random = 0x53a9e17u;
		u64 m_serial = 0;
};

#endif // SOUNDMANAGER_HPP_
