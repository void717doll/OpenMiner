/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#include "common/world/WorldFiles.hpp"
#include <filesystem.hpp>
#include <random.hpp>

#include "common/core/ApplicationStateStack.hpp"
#include "common/core/Debug.hpp"
#include "common/core/Utils.hpp"

#include "client/core/Config.hpp"
#include "client/states/TitleScreenState.hpp"
#include "client/states/WorldCreationState.hpp"
#include "client/states/WorldSelectionState.hpp"

using Random = effolkronium::random_static;

namespace fs = ghc::filesystem;

WorldCreationState::WorldCreationState(TitleScreenState *titleScreen, const std::string &originalName) : InterfaceState(titleScreen) {
	if (!originalName.empty())
		m_isEdition = true;

	m_background.setScale(Config::guiScale * 2.f, Config::guiScale * 2.f);

	m_filter.setFillColor(Color::fromRGBA32(0, 0, 0, 192));

	m_title.setScale(Config::guiScale, Config::guiScale);
	m_title.setString(originalName.empty() ? "Create New World" : "Edit World");
	m_title.updateVertexBuffer();

	m_nameInput.setString(originalName);
	m_nameInput.setCharacterLimit(32);
	m_nameInput.setBackgroundSize(150, 20);
	m_nameInput.setBackgroundOutline(1, Color::White);
	m_nameInput.setPadding(5, 6);
	m_nameInput.setScale(Config::guiScale, Config::guiScale);
	m_nameInput.setFocus(false);
	m_nameInput.setPlaceholder("World Name (Optional)");

	if (!m_isEdition) {
		m_seedInput.setString(originalName);
		m_seedInput.setCharacterLimit(32);
		m_seedInput.setBackgroundSize(150, 20);
		m_seedInput.setBackgroundOutline(1, Color::White);
		m_seedInput.setPadding(5, 6);
		m_seedInput.setScale(Config::guiScale, Config::guiScale);
		m_seedInput.setFocus(false);
		m_seedInput.setPlaceholder("Seed (random if blank)");
		// m_seedInput.setString(std::to_string(originalSeed));
		// FIXME: Find a way to retrieve original seed
	}

	m_modeButton.setScale(Config::guiScale, Config::guiScale);
	m_modeButton.setText("Game Mode: Survival");
	m_modeButton.setCallback([this](TextButton &button) {
		m_creative = !m_creative;
		button.setText(m_creative ? "Game Mode: Creative" : "Game Mode: Survival");
	});

	m_menuWidget.setScale(Config::guiScale, Config::guiScale);
	m_menuWidget.addButton(originalName.empty() ? "Create New World" : "Save Changes", [this, titleScreen, originalName](TextButton &) {
		std::string worldName = m_nameInput.string();
		auto first = worldName.find_first_not_of(" ");
		worldName = first == std::string::npos ? "" : worldName.substr(first, worldName.find_last_not_of(" ") - first + 1);
		if (worldName.empty()) {
			unsigned number = 1;
			do { worldName = "World " + std::to_string(number++); }
			while (worldFiles::exists(worldName));
		}
		if (!worldFiles::exists(worldName)) {
			if (utils::regexMatch(worldName, "^[A-Za-z0-9_ ]+$") && worldName[0] != '_') {
				if (m_isEdition) {
					if (fs::exists(worldFiles::database(originalName)))
						fs::rename(worldFiles::directory(originalName), worldFiles::directory(worldName));
					else fs::rename(worldFiles::legacy(originalName), worldFiles::legacy(worldName));

					m_stateStack->pop();

					// FIXME: This is needed because there's currently no way to refresh WorldSelectionState
					m_stateStack->pop(); // WorldSelectionState
					m_stateStack->push<WorldSelectionState>(titleScreen);
				}
				else
					m_stateStack->pop();

				if (!m_isEdition) {
					Random::seed((unsigned int)std::time(nullptr));
					s32 seed = Random::get<s32>(std::numeric_limits<s32>::min(), std::numeric_limits<s32>::max());
					if (!m_seedInput.string().empty()) {
						try {
							seed = std::stoi(m_seedInput.string());
						}
						catch (...) {
							logError() << "Invalid seed, using random one:" << seed;
						}
					}

					titleScreen->startSingleplayer(true, worldName, seed, m_creative);
				}
			}
			else {
				m_errorText.setString("Invalid world name");
				m_errorText.updateVertexBuffer();
				updateWidgetPosition();
			}
		}
		else {
			m_errorText.setString("World '" + worldName + "' already exists");
			m_errorText.updateVertexBuffer();
			updateWidgetPosition();
		}
	}, 150);

	m_menuWidget.addButton("Cancel", [this](TextButton &) {
		m_stateStack->pop();
	}, 150);

	m_errorText.setColor(Color::Red);
	m_errorText.setScale(Config::guiScale, Config::guiScale);

	updateWidgetPosition();
}

void WorldCreationState::onEvent(const SDL_Event &event) {
	InterfaceState::onEvent(event);

	if (!m_stateStack->empty() && &m_stateStack->top() == this) {
		m_nameInput.onEvent(event);

		if (!m_isEdition) {
			m_seedInput.onEvent(event);
			m_modeButton.onEvent(event);
		}

		m_menuWidget.onEvent(event);
	}
}

void WorldCreationState::update() {
}

void WorldCreationState::updateWidgetPosition() {
	m_background.setPosRect(0, 0,
		u16(Config::screenWidth / m_background.getScale().x),
		u16(Config::screenHeight / m_background.getScale().y)
	);

	m_background.setClipRect(0, 0,
		u16(Config::screenWidth / m_background.getScale().x),
		u16(Config::screenHeight / m_background.getScale().y)
	);

	m_filter.setSize(Config::screenWidth, Config::screenHeight);

	m_title.setPosition(
		Config::screenWidth / 2.0f - float(m_title.getSize().x * Config::guiScale) / 2.0f,
		12.5f * Config::guiScale - float(m_title.getSize().y * Config::guiScale) / 2.0f
	);

	if (!m_isEdition) {
		m_modeButton.setPosition(Config::screenWidth / 2.f - 75 * Config::guiScale, Config::screenHeight / 2.f + 20 * Config::guiScale);
		m_nameInput.setPosition(
			Config::screenWidth / 2.0f - float(m_nameInput.getBackgroundSize().x * Config::guiScale) / 2.0f,
			Config::screenHeight / 2.0f - float(m_nameInput.getBackgroundSize().y * Config::guiScale) / 2.0f - 30 * Config::guiScale
		);

		m_seedInput.setPosition(
			Config::screenWidth / 2.0f - float(m_seedInput.getBackgroundSize().x * Config::guiScale) / 2.0f,
			Config::screenHeight / 2.0f - float(m_seedInput.getBackgroundSize().y * Config::guiScale) / 2.0f
		);
	}
	else {
		m_modeButton.setPosition(Config::screenWidth / 2.f - 75 * Config::guiScale, Config::screenHeight / 2.f + 20 * Config::guiScale);
		m_nameInput.setPosition(
			Config::screenWidth / 2.0f - float(m_nameInput.getBackgroundSize().x * Config::guiScale) / 2.0f,
			Config::screenHeight / 2.0f - float(m_nameInput.getBackgroundSize().y * Config::guiScale) / 2.0f
		);
	}

	m_menuWidget.setPosition(
		Config::screenWidth / 2.0f - m_menuWidget.getGlobalBounds().sizeX / 2.0f,
		Config::screenHeight - 12.5f * Config::guiScale - m_menuWidget.getGlobalBounds().sizeY / 2.0f
	);

	m_errorText.setPosition(
		Config::screenWidth / 2.0f - float(m_errorText.getSize().x * Config::guiScale) / 2.0f,
		Config::screenHeight / 2.0f - 30 * Config::guiScale
	);
}

void WorldCreationState::draw(RenderTarget &target, RenderStates states) const {
	if (&m_stateStack->top() == this) {
		prepareDraw(target, states);

		target.draw(m_background, states);
		target.draw(m_filter, states);

		target.draw(m_title, states);

		target.draw(m_nameInput, states);

		if (!m_isEdition) {
			target.draw(m_seedInput, states);
			target.draw(m_modeButton, states);
		}

		target.draw(m_menuWidget, states);

		if (!m_errorText.string().empty())
			target.draw(m_errorText, states);
	}
}
