#include "common/core/ApplicationStateStack.hpp"
#include "client/core/Config.hpp"
#include "client/core/Mouse.hpp"
#include "client/network/ClientCommandHandler.hpp"
#include "client/states/DeathState.hpp"
#include "client/states/WorldSavingState.hpp"
#include "client/world/ClientPlayer.hpp"

DeathState::DeathState(ClientCommandHandler &client, ClientPlayer &player, DrawableState *parent)
	: InterfaceState(parent), m_client(client), m_player(player) {
	m_drawBackground = false;
	Mouse::setCursorGrabbed(false);
	Mouse::setCursorVisible(true);
	m_filter.setFillColor(Color::fromRGBA32(96, 0, 0, 160));
	m_title.setString("You Died!");
	m_scoreLabel.setString("Score: ");
	m_scoreValue.setString(std::to_string(player.score()));
	m_scoreValue.setColor(Color::fromRGBA32(255, 255, 85));
	m_menu.setVerticalSpacing(4);
	m_menu.addButton("Respawn", [this](TextButton &) {
		if (m_waiting) return;
		m_waiting = true;
		m_menu.setButtonEnabled("Respawn", false);
		m_client.sendPlayerRespawn();
	});
	m_menu.addButton("Title Screen", [this](TextButton &) {
		m_stateStack->push<WorldSavingState>(m_client.client(), false);
	});
	updateWidgetPosition();
}

void DeathState::onEvent(const SDL_Event &event) {
	InterfaceState::onEvent(event);
	if (!m_stateStack->empty() && &m_stateStack->top() == this) m_menu.onEvent(event);
}

void DeathState::update() {
	InterfaceState::update();
	if (m_player.health() && !m_stateStack->empty() && &m_stateStack->top() == this) {
		Mouse::setCursorGrabbed(true);
		Mouse::setCursorVisible(false);
		Mouse::resetToWindowCenter();
		m_stateStack->pop();
	}
}

void DeathState::updateWidgetPosition() {
	float scale = float(std::min<int>(Config::guiScale, std::max(1, Config::screenHeight / 240)));
	m_filter.setSize(Config::screenWidth, Config::screenHeight);
	m_title.updateVertexBuffer();
	m_scoreLabel.updateVertexBuffer();
	m_scoreValue.updateVertexBuffer();
	m_title.setScale(scale * 2, scale * 2);
	m_title.setPosition(Config::screenWidth / 2.f - m_title.getSize().x * scale, 60 * scale);
	m_scoreLabel.setScale(scale, scale);
	m_scoreValue.setScale(scale, scale);
	float left = (Config::screenWidth - (m_scoreLabel.getSize().x + m_scoreValue.getSize().x) * scale) / 2;
	m_scoreLabel.setPosition(left, 100 * scale);
	m_scoreValue.setPosition(left + m_scoreLabel.getSize().x * scale, 100 * scale);
	m_menu.setScale(scale, scale);
	m_menu.setPosition((Config::screenWidth - 200 * scale) / 2, Config::screenHeight / 4.f + 72 * scale);
}

void DeathState::draw(RenderTarget &target, RenderStates states) const {
	if (m_parent) target.draw(*static_cast<DrawableState *>(m_parent), states);
	prepareDraw(target, states);
	target.draw(m_filter, states);
	target.draw(m_title, states);
	target.draw(m_scoreLabel, states);
	target.draw(m_scoreValue, states);
	target.draw(m_menu, states);
}
