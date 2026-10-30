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
#ifndef SERVERPLAYER_HPP_
#define SERVERPLAYER_HPP_

#include <unordered_set>

#include "common/core/Debug.hpp"
#include "common/world/Player.hpp"
#include <optional>
#include "server/network/ClientInfo.hpp"
#include "server/world/PlayerSurvival.hpp"

class ServerPlayer : public Player {
	public:
		ServerPlayer(const std::string &name, bool isNewPlayer) {
			m_name = name;
			m_isNewPlayer = isNewPlayer;
		}

		const ClientInfo *client() const { return m_client; }
		void setClient(ClientInfo *client) { m_client = client; }

		bool isOnline() const { return m_client != nullptr; }
		bool isNewPlayer() const { return m_isNewPlayer; }
		void setNewPlayer(bool isNewPlayer) { m_isNewPlayer = isNewPlayer; }

		bool isRidingMinecart() const { return m_ridingMinecart; }
		void setRidingMinecart(bool mounted) { m_ridingMinecart = mounted; }

		bool isReady() const { return m_isReady; }
		void setReady(bool isReady) { m_isReady = isReady; }
		PlayerSurvivalState &survival() { return m_survival; }
		const PlayerSurvivalState &survival() const { return m_survival; }
		void resetSurvival() { m_survival = {}; }
		const std::optional<Vector3i> &bedSpawn() const { return m_bedSpawn; }
		void setBedSpawn(const Vector3i &position) { m_bedSpawn = position; }
		void clearBedSpawn() { m_bedSpawn.reset(); }

		bool isChunkLoaded(const Vector3i &chunk) const { return m_loadedChunks.find(chunk) != m_loadedChunks.end(); }
		void addLoadedChunk(const Vector3i &chunk) { m_loadedChunks.emplace(chunk); }
		void removeLoadedChunk(const Vector3i &chunk) { m_loadedChunks.erase(chunk); }
		void clearLoadedChunks() { m_loadedChunks.clear(); }
		const std::unordered_set<Vector3i> &loadedChunks() const { return m_loadedChunks; }

		bool tryPortal(u64 now) {
			if (now < m_nextPortal) return false;
			m_nextPortal = now + 1500;
			return true;
		}

		bool tryAttack(u64 now) {
			if (now < m_nextAttack) return false;
			m_nextAttack = now + 400;
			return true;
		}

		static void initUsertype(sol::state &lua);

	public:
		std::unordered_set<Vector3i> sentChunks; // Cleared every tick

	private:
		std::optional<Vector3i> m_bedSpawn;
		ClientInfo *m_client = nullptr;
		u64 m_nextAttack = 0;
		u64 m_nextPortal = 0;
		PlayerSurvivalState m_survival;

		bool m_ridingMinecart = false;
		bool m_isNewPlayer = false;
		bool m_isReady = false; // Is player ready to receive chunks?

		std::unordered_set<Vector3i> m_loadedChunks;
};

#endif // SERVERPLAYER_HPP_
