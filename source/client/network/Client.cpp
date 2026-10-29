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
#include "common/core/Exception.hpp"
#include "client/network/Client.hpp"

void Client::connect(Player &player) {
	disconnect();
	m_session = Network::LocalSession::connect();
	if (!m_session)
		throw ClientConnectException("local world is unavailable");

	Network::Packet packet;
	packet << Network::Command::ClientConnect << player.name();
	m_session->sendToServer(packet);

	Network::Packet answer;
	if (!m_session->waitOnClient(answer, std::chrono::seconds(5))) {
		m_session->close();
		throw ClientConnectException("local world did not respond");
	}

	Network::Command command;
	answer >> command >> m_id;
	if (!answer || command != Network::Command::ClientOk) {
		m_session->close();
		throw ClientConnectException("invalid local world session");
	}

	m_isConnected = true;
}

void Client::disconnect() {
	if (!m_session || !m_isConnected)
		return;

	Network::Packet packet;
	packet << Network::Command::ClientDisconnect;
	m_session->sendToServer(packet);
	m_isConnected = false;
}

void Client::send(Network::Packet &packet) {
	if (!m_session || !m_isConnected)
		throw EXCEPTION("local world is not connected");
	m_session->sendToServer(packet);
}

void Client::update() {
	if (!m_session)
		return;

	// callbacks may remove the state that owns this client.
	auto session = m_session;
	Network::Packet packet;
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
	unsigned processed = 0;
	while (processed < 64 && std::chrono::steady_clock::now() < deadline && session->receiveOnClient(packet)) {
		++processed;
		Network::Command command;
		packet >> command;
		if (!packet)
			continue;

		auto it = m_commands.find(command);
		if (it != m_commands.end()) {
			auto callback = it->second;
			callback(packet);
			if (command == Network::Command::ServerClosed)
				return;
		}
	}
}
