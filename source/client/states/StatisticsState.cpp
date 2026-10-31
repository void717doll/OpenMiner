#include <algorithm>
#include <iomanip>
#include <sstream>
#include "common/core/ApplicationStateStack.hpp"
#include "common/core/Registry.hpp"
#include "client/core/Config.hpp"
#include "client/core/GameConfig.hpp"
#include "client/network/Client.hpp"
#include "client/world/ClientPlayer.hpp"
#include "client/states/StatisticsState.hpp"

namespace {
std::string distance(u64 centimeters) {
	std::ostringstream text;
	text << std::fixed << std::setprecision(2);
	if (centimeters >= 100000) text << double(centimeters) / 100000 << " km";
	else text << double(centimeters) / 100 << " m";
	return text.str();
}
}

StatisticsState::StatisticsState(Client &client, const ClientPlayer &player, DrawableState *parent)
	: InterfaceState(parent), m_player(player) {
	m_title.setString("Statistics");
	m_worldName.setString(GameConfig::worldName);
	m_worldName.setColor(Color::fromRGBA32(180, 180, 180));
	m_background.setColor(Color::fromRGBA32(70, 70, 70));
	m_panel.setFillColor(Color::fromRGBA32(0, 0, 0, 120));
	m_scrollbar.setFillColor(Color::fromRGBA32(150, 150, 150));
	const char *tabs[] = {"General", "Blocks", "Items"};
	for (int tab = 0; tab < 3; ++tab) {
		m_tabs.addButton(tabs[tab], [this, tab](TextButton &) {
			m_tab = tab;
			m_offset = 0;
			refresh();
		}, 90);
	}
	m_done.setText("Done");
	m_done.setCallback([this](TextButton &) { m_stateStack->pop(); });
	refresh();
	updateWidgetPosition();
	Network::Packet packet;
	packet << Network::Command::PlayerStatistics;
	client.send(packet);
}

void StatisticsState::refresh() {
	const auto &stats = m_player.statistics();
	m_revision = stats.revision;
	m_rows.clear();
	if (!m_tab) {
		static const char *names[] = {"Time Played", "Distance Walked", "Distance Swum", "Distance Fallen",
			"Distance Climbed", "Distance Flown", "Distance by Minecart", "Jumps", "Items Dropped",
			"Damage Dealt", "Damage Taken", "Deaths", "Mob Kills"};
		for (int i = 0; i < PlayerStatistics::Count; ++i) {
			u64 value = stats.general[i];
			std::string formatted = std::to_string(value);
			if (i == PlayerStatistics::PlayTicks) {
				u64 seconds = value / 20;
				formatted = std::to_string(seconds / 3600) + "h " + std::to_string(seconds / 60 % 60) + "m " + std::to_string(seconds % 60) + "s";
			} else if (i >= PlayerStatistics::WalkCm && i <= PlayerStatistics::CartCm) formatted = distance(value);
			m_rows.push_back({names[i], {formatted, "", ""}});
		}
	} else {
		auto &definitions = Registry::getInstance();
		for (const auto &entry : stats.items) {
			if (!definitions.hasItem(entry.first)) continue;
			const auto &item = definitions.getItemFromStringID(entry.first);
			bool block = definitions.hasBlock(entry.first);
			if ((m_tab == 1) != block) continue;
			const auto &n = entry.second;
			m_rows.push_back({item.label(), {std::to_string(m_tab == 1 ? n.mined : n.used),
				std::to_string(n.crafted), std::to_string(m_tab == 1 ? n.placed : n.broken)}});
		}
		std::sort(m_rows.begin(), m_rows.end(), [](const Row &a, const Row &b) { return a.name < b.name; });
	}
	m_tabs.setButtonEnabled("General", m_tab != 0);
	m_tabs.setButtonEnabled("Blocks", m_tab != 1);
	m_tabs.setButtonEnabled("Items", m_tab != 2);
	updateRowLabels();
}

void StatisticsState::onEvent(const SDL_Event &event) {
	InterfaceState::onEvent(event);
	if (m_stateStack->empty() || &m_stateStack->top() != this) return;
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) { m_stateStack->pop(); return; }
	if (event.type == SDL_MOUSEWHEEL) {
		int visible = visibleRows();
		m_offset = std::clamp(m_offset - event.wheel.y * 3, 0, std::max(0, int(m_rows.size()) - visible));
		updateRowLabels();
	}
	m_tabs.onEvent(event);
	m_done.onEvent(event);
}

void StatisticsState::update() {
	InterfaceState::update();
	if (m_revision != m_player.statistics().revision) refresh();
}

void StatisticsState::updateWidgetPosition() {
	m_scale = std::max(1, std::min({int(Config::guiScale), int(Config::screenWidth) / 320, int(Config::screenHeight) / 200}));
	float scale = m_scale;
	m_background.setScale(scale * 2, scale * 2);
	m_background.setPosRect(0, 0, Config::screenWidth / (scale * 2), Config::screenHeight / (scale * 2));
	m_background.setClipRect(0, 0, Config::screenWidth / (scale * 2), Config::screenHeight / (scale * 2));
	m_title.setScale(scale, scale);
	m_worldName.setScale(scale, scale);
	m_tabs.setScale(scale, scale);
	m_done.setScale(scale, scale);
	m_title.updateVertexBuffer();
	m_worldName.updateVertexBuffer();
	m_title.setPosition((Config::screenWidth - m_title.getSize().x * scale) / 2, 10 * scale);
	m_worldName.setPosition((Config::screenWidth - m_worldName.getSize().x * scale) / 2, 23 * scale);
	m_tabs.setPosition((Config::screenWidth - m_tabs.getGlobalBounds().sizeX) / 2, Config::screenHeight - 52 * scale);
	m_done.setPosition((Config::screenWidth - m_done.getGlobalBounds().sizeX) / 2, Config::screenHeight - 26 * scale);
	m_panel.setPosition(0, 35 * scale);
	m_panel.setSize(Config::screenWidth, std::max(0.f, Config::screenHeight - 92 * scale));
	updateRowLabels();
}

int StatisticsState::visibleRows() const {
	return std::max(1, (int(Config::screenHeight) / m_scale - 58 - (m_tab ? 56 : 40)) / (m_tab ? 18 : 10));
}

void StatisticsState::updateRowLabels() {
	m_labels.clear();
	float scale = m_scale;
	float left = Config::screenWidth / 2.f - 145 * scale;
	auto text = [&](const std::string &value, float x, float y, bool right = false, bool dim = false) {
		auto label = std::make_unique<Text>();
		label->setScale(scale, scale);
		label->setString(value);
		if (dim) label->setColor(Color::fromRGBA32(170, 170, 170));
		label->updateVertexBuffer();
		label->setPosition(x - (right ? label->getSize().x * scale : 0), y);
		m_labels.push_back(std::move(label));
	};
	if (m_tab) {
		text(m_tab == 1 ? "Mined" : "Used", left + 190 * scale, 40 * scale, true);
		text("Crafted", left + 240 * scale, 40 * scale, true);
		text(m_tab == 1 ? "Placed" : "Broken", left + 290 * scale, 40 * scale, true);
	}
	int visible = visibleRows();
	m_offset = std::clamp(m_offset, 0, std::max(0, int(m_rows.size()) - visible));
	float top = (m_tab ? 56 : 40) * scale;
	float height = visible * (m_tab ? 18 : 10) * scale;
	float thumb = m_rows.size() > unsigned(visible) ? height * visible / m_rows.size() : 0;
	m_scrollbar.setSize(2 * scale, thumb);
	m_scrollbar.setPosition(left + 300 * scale, top + (thumb ? (height - thumb) * m_offset / (m_rows.size() - visible) : 0));
	if (m_rows.empty()) text("No statistics yet", left, 56 * scale);
	for (int row = 0; row < visible && row + m_offset < int(m_rows.size()); ++row) {
		const auto &entry = m_rows[row + m_offset];
		float y = top + row * (m_tab ? 18 : 10) * scale;
		bool dim = (row + m_offset) % 2;
		std::string name = entry.name;
		if (m_tab && name.size() > 24) name = name.substr(0, 21) + "...";
		text(name, left, y, false, dim);
		for (int column = 0; column < (m_tab ? 3 : 1); ++column)
			text(entry.values[column], left + (m_tab ? 190 + column * 50 : 290) * scale, y, true, dim);
	}
}

void StatisticsState::draw(RenderTarget &target, RenderStates states) const {
	prepareDraw(target, states);
	target.draw(m_background, states);
	target.draw(m_panel, states);
	target.draw(m_scrollbar, states);
	target.draw(m_title, states);
	target.draw(m_worldName, states);
	for (const auto &label : m_labels) target.draw(*label, states);
	target.draw(m_tabs, states);
	target.draw(m_done, states);
	target.disableView();
}
