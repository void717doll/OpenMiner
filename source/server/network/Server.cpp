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
#include "server/network/Server.hpp"

Server::~Server() {
	if (m_session)
		m_session->close();
}

void Server::init() {
	m_session = Network::LocalSession::create();
}

void Server::disconnectAllClients() {
	if (!m_info.clients().empty())
		disconnectClient(m_info.clients().front());
	if (m_session)
		m_session->close();
	m_isRunning = false;
}

void Server::disconnectClient(ClientInfo &client) {
	const u16 id = client.id;
	Network::Packet packet;
	auto it = m_commands.find(Network::Command::ClientDisconnect);
	if (it != m_commands.end())
		it->second(client, packet);

	m_info.removeClient(id);
	// saving finishes after the loop exits; its acknowledgement still needs the queue.
	m_isRunning = false;
}

void Server::handleGameEvents() {
	m_session->waitOnServer(std::chrono::milliseconds(10));
	handleClientMessages();
}

void Server::handleClientMessages() {
	Network::Packet packet;
	int processed = 0;
	for (; processed < 100 && m_session->receiveOnServer(packet); ++processed) {
		Network::Command command;
		packet >> command;
		if (!packet)
			continue;

		if (m_info.clients().empty()) {
			if (command != Network::Command::ClientConnect)
				continue;

			ClientInfo &client = m_info.addClient(m_session);
			Network::Packet answer;
			answer << Network::Command::ClientOk << client.id;
			client.send(answer);
			if (m_connectionCallback)
				m_connectionCallback(client, packet);
		}
		else if (command == Network::Command::ClientDisconnect) {
			disconnectClient(m_info.clients().front());
			return;
		}
		else {
			auto it = m_commands.find(command);
			if (it != m_commands.end())
				it->second(m_info.clients().front(), packet);
		}

		if (!m_isRunning)
			return;
	}

	if (processed == 100)
		return;

	if (m_session->isClosed())
		disconnectAllClients();
}

void Server::sendToAllClients(Network::Packet &packet) const {
	if (m_session)
		m_session->sendToClient(packet);
}
