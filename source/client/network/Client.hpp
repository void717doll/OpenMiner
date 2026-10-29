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
#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <functional>
#include <unordered_map>

#include "common/network/LocalSession.hpp"
#include "common/world/Player.hpp"

class ClientConnectException {
	public:
		ClientConnectException(const std::string &str) : m_str(str) {}
		const char *what() const noexcept { return m_str.c_str(); }

	private:
		std::string m_str;
};

class Client {
	using CommandCallback = std::function<void(Network::Packet &packet)>;

	public:
		~Client() { disconnect(); }

		void connect(Player &player);
		void disconnect();
		void send(Network::Packet &packet);
		void update();

		void setCommandCallback(Network::Command command, const CommandCallback &callback) { m_commands[command] = callback; }

		bool isConnected() const { return m_isConnected; }
		u16 id() const { return m_id; }

	private:
		bool m_isConnected = false;
		u16 m_id = 0;
		std::shared_ptr<Network::LocalSession> m_session;
		std::unordered_map<Network::Command, CommandCallback> m_commands;
};

#endif // CLIENT_HPP_
