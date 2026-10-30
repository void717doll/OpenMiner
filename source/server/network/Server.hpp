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
#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <functional>
#include <unordered_map>

#include "common/network/Network.hpp"
#include "server/network/ServerInfo.hpp"

class Server {
	using ConnectionCallback = std::function<void(ClientInfo &, Network::Packet &packet)>;
	using CommandCallback = std::function<void(ClientInfo &, Network::Packet &packet)>;

	public:
		~Server();
		void init();

		void disconnectAllClients();
		void disconnectClient(ClientInfo &client);

		void handleGameEvents();

		void sendToAllClients(Network::Packet &packet) const;

		bool isRunning() const { return m_isRunning; }
		bool isPaused() const { return m_isPaused; }
		void setPaused(bool paused) { m_isPaused = paused; }

		const ServerInfo &info() const { return m_info; }

		void setRunning(bool isRunning) { m_isRunning = isRunning; }

		void setConnectionCallback(const ConnectionCallback &callback) { m_connectionCallback = callback; }
		void setCommandCallback(Network::Command command, const CommandCallback &callback) { m_commands[command] = callback; }

	private:
		void handleClientMessages();

		bool m_isRunning = false;
		bool m_isPaused = false;

		ServerInfo m_info;

		std::shared_ptr<Network::LocalSession> m_session;

		ConnectionCallback m_connectionCallback;

		std::unordered_map<Network::Command, CommandCallback> m_commands;
};

#endif // SERVER_HPP_
