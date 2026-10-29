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
#include "common/core/EngineConfig.hpp"
#include "common/core/GameClock.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/core/ServerConfig.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerPlayer.hpp"
#include <array>
#include <chrono>

#include "server/world/ServerWorld.hpp"
#include "common/world/BetaPlayerMovement.hpp"

ServerWorld::ServerWorld(PlayerList &players, const Dimension &dimension, s32 seed)
	: m_players(players),
	  m_dimension(dimension),
	  m_heightmap(seed, dimension.id() == 1),
	  m_terrainGenerator(m_heightmap, dimension, seed),
	  m_scene(players),
	  m_seed(seed)
{
}

void ServerWorld::update(bool doTick) {
	World::update();

	processSendRequests();

	if (doTick) {
		for (auto &it : m_chunks) // TODO: Loop on a subset of this hashmap
			it.second->tick();

		m_blockPhysics.tick();
		updateMobs();
		updateItems();
		updateTnt();
		updatePlayerSurvival();
		m_scene.update();
	}
}

bool ServerWorld::moveSurvivalPlayer(ServerPlayer &player, double x, double y, double z) {
	if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()
	 || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
	 || std::abs(x) > 1000000000 || std::abs(y) > 1000000000 || std::abs(z) > 1000000000) return false;
	Vector3d requested{x - player.x(), y - player.y(), z - player.z()};
	if (requested.length() > 64) return false;
	DoubleBox box = player.hitbox();
	box.move(player.x(), player.y(), player.z());
	if (!playerEnvironment(*this, box).loaded) { player.survival().fallDistance = 0; return false; }
	Vector3d allowed = retro::movePlayer(*this, box, requested, false, false);
	if ((allowed - requested).length() > 0.002) return false;
	box = player.hitbox(); box.move(x, y, z);
	PlayerEnvironment environment = playerEnvironment(*this, box);
	if (!environment.loaded) { player.survival().fallDistance = 0; return false; }
	player.setPosition(x, y, z);
	u16 damage = player.survival().moved(requested.z, environment.grounded, environment.water);
	if (damage) {
		if (m_server) m_server->damagePlayer(player.clientID(), damage);
		else player.damage(player.survival().filterDamage(damage));
	}
	return true;
}

void ServerWorld::updatePlayerSurvival() {
	for (auto &entry : m_players) {
		auto &player = entry.second;
		if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
		DoubleBox box = player.hitbox(); box.move(player.x(), player.y(), player.z());
		u16 damage = player.survival().tick(playerEnvironment(*this, box));
		if (damage) {
			if (m_server) m_server->damagePlayer(player.clientID(), damage);
			else player.damage(player.survival().filterDamage(damage));
		}
	}
}

void ServerWorld::updatePlayerChunks(ServerPlayer &player, s32 cx, s32 cy, s32 cz) {
	Vector3i currentChunk{cx, cy, cz};

	player.sentChunks.clear();
	m_chunkSendRequestQueue = {};
	m_queuedChunks.clear();
	m_queuedChunks.emplace(currentChunk);
	m_chunkSendRequestQueue.emplace(currentChunk, player, GameClock::getInstance().getTicks(true));

	std::list<Vector3i> chunksToRemove;
	for (auto &chunkPos : player.loadedChunks()) {
		glm::dvec3 chunkWorldPos{
			chunkPos.x * CHUNK_WIDTH + CHUNK_WIDTH / 2,
			chunkPos.y * CHUNK_DEPTH + CHUNK_DEPTH / 2,
			chunkPos.z * CHUNK_HEIGHT + CHUNK_HEIGHT / 2
		};

		glm::dvec3 playerPos{
			player.x(),
			player.y(),
			player.z()
		};

		if (glm::length(playerPos - chunkWorldPos) >= (ServerConfig::renderDistance + 3) * CHUNK_WIDTH) {
			m_server->sendChunkUnload(chunkPos.x, chunkPos.y, chunkPos.z, player.client());
			chunksToRemove.emplace_back(chunkPos);

			// logDebug() << "Chunk at" << chunkPos.x << chunkPos.y << chunkPos.z << "unloaded for player" << player.name();
		}
	}

	for (auto &chunkPos : chunksToRemove)
		player.removeLoadedChunk(chunkPos);
}

void ServerWorld::createChunkNeighbours(ServerChunk &chunk) {
	Vector3i surroundingChunks[6] = {
		{chunk.x() - 1, chunk.y(),     chunk.z()},
		{chunk.x() + 1, chunk.y(),     chunk.z()},
		{chunk.x(),     chunk.y() - 1, chunk.z()},
		{chunk.x(),     chunk.y() + 1, chunk.z()},
		{chunk.x(),     chunk.y(),     chunk.z() - 1},
		{chunk.x(),     chunk.y(),     chunk.z() + 1},
	};

	for (u8 i = 0 ; i < 6 ; ++i) {
		// Check if this neighbour already exists, if yes then skip it
		ServerChunk *neighbour = (ServerChunk *)getChunk(surroundingChunks[i].x, surroundingChunks[i].y, surroundingChunks[i].z);
		if (neighbour) {
			// Assign surrounding chunk pointers
			chunk.setSurroundingChunk(i, neighbour);
			neighbour->setSurroundingChunk((i % 2 == 0) ? i + 1 : i - 1, &chunk);

			continue;
		}

		// Create our neighbour
		auto it = m_chunks.emplace(
			Vector3i{
				surroundingChunks[i].x,
				surroundingChunks[i].y,
				surroundingChunks[i].z
			},
			new ServerChunk{
				surroundingChunks[i].x,
				surroundingChunks[i].y,
				surroundingChunks[i].z,
				*this
			}
		);

		// Get the created neighbour
		neighbour = it.first->second.get();

		// Assign surrounding chunk pointers
		chunk.setSurroundingChunk(i, neighbour);
		neighbour->setSurroundingChunk((i % 2 == 0) ? i + 1 : i - 1, &chunk);
	}
}

ServerChunk &ServerWorld::getOrCreateChunk(s32 cx, s32 cy, s32 cz) {
	ServerChunk *chunk = (ServerChunk *)getChunk(cx, cy, cz);
	if (!chunk) {
		auto it = m_chunks.emplace(Vector3i{cx, cy, cz}, new ServerChunk(cx, cy, cz, *this));
		chunk = it.first->second.get();
	}

	// Create our neighbours
	createChunkNeighbours(*chunk);

	return *chunk;
}

Chunk *ServerWorld::getChunk(int cx, int cy, int cz) const {
	auto it = m_chunks.find({cx, cy, cz});
	if (it == m_chunks.end())
		return nullptr;

	return it->second.get();
}

bool ServerWorld::generateChunk(ServerChunk &chunk) {
	if (chunk.isInitialized()) return false;

	createChunkNeighbours(chunk);

	m_terrainGenerator.generate(chunk);
	chunk.setInitialized(true);
	m_blockPhysics.seedChunk(chunk);

	return true;
}

void ServerWorld::processSendRequests() {
	using Clock = std::chrono::steady_clock;
	auto deadline = Clock::now() + std::chrono::milliseconds(3);
	unsigned generated = 0;
	unsigned processed = 0;
	while (!m_chunkSendRequestQueue.empty() && processed < 32 && Clock::now() < deadline) {
		auto &request = m_chunkSendRequestQueue.front();
		Vector3i position = request.chunkPos;
		auto &player = request.player;
		bool expired = GameClock::getInstance().getTicks(true) - request.timestamp >= 10000;
		Vector3d center{position.x * CHUNK_WIDTH + CHUNK_WIDTH / 2.0,
			position.y * CHUNK_DEPTH + CHUNK_DEPTH / 2.0, position.z * CHUNK_HEIGHT + CHUNK_HEIGHT / 2.0};
		Vector3d difference = center - Vector3d{player.x(), player.y(), player.z()};
		bool inRange = difference.length() < (ServerConfig::renderDistance + 2) * CHUNK_WIDTH;
		if (!expired && inRange && player.isOnline() && !player.sentChunks.count(position)) {
			if (!player.isChunkLoaded(position)) {
				ServerChunk &chunk = getOrCreateChunk(position.x, position.y, position.z);
				std::array<Chunk *, 7> required{{&chunk, chunk.getSurroundingChunk(Chunk::West),
					chunk.getSurroundingChunk(Chunk::East), chunk.getSurroundingChunk(Chunk::South),
					chunk.getSurroundingChunk(Chunk::North), chunk.getSurroundingChunk(Chunk::Bottom),
					chunk.getSurroundingChunk(Chunk::Top)}};
				bool ready = true;
				for (Chunk *neighbor : required) {
					if (neighbor->isInitialized()) continue;
					if (!generated && Clock::now() < deadline) {
						generateChunk(static_cast<ServerChunk &>(*neighbor));
						++generated;
					} else { ready = false; break; }
				}
				// keep the request until its neighbors can supply light and mesh borders.
				if (!ready) break;
				chunk.lightmap().updateLights();
				chunk.setReadyToSend();
				addChunkToProcess(&chunk);
				player.addLoadedChunk(position);
			}
			player.sentChunks.emplace(position);
			for (const Vector3i &offset : {Vector3i{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}}) {
				Vector3i neighbor = position + offset;
				if (!player.sentChunks.count(neighbor) && m_queuedChunks.emplace(neighbor).second)
					m_chunkSendRequestQueue.emplace(neighbor, player, GameClock::getInstance().getTicks(true));
			}
		}
		m_queuedChunks.erase(position);
		m_chunkSendRequestQueue.pop();
		++processed;
	}
}

void ServerWorld::initUsertype(sol::state &lua) {
	lua.new_usertype<ServerWorld>("ServerWorld",
		sol::base_classes, sol::bases<World>(),

		"dimension", &ServerWorld::dimension
	);
}

void ServerWorld::appendCollisionBoxes(const DoubleBox &moving, const DoubleBox &broadphase, std::vector<DoubleBox> &boxes) const {
	auto add = [&](DoubleBox box) {
		if (std::abs(box.x - moving.x) < 1e-6 && std::abs(box.y - moving.y) < 1e-6 && std::abs(box.z - moving.z) < 1e-6
		 && std::abs(box.sizeX - moving.sizeX) < 1e-6 && std::abs(box.sizeY - moving.sizeY) < 1e-6
		 && std::abs(box.sizeZ - moving.sizeZ) < 1e-6) return;
		if (box.intersects(broadphase)) boxes.push_back(box);
	};
	for (const auto &entry : m_players) {
		const auto &player = entry.second;
		if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
		DoubleBox box = player.hitbox();
		box.move(player.x(), player.y(), player.z());
		add(box);
	}
	m_scene.registry().view<const MobComponent, const PositionComponent, const DoubleBox>().each(
		[&](auto entity, const auto &, const auto &position, const auto &localBox) {
			const auto *traits = m_scene.registry().try_get<BetaMobComponent>(entity);
			if (position.dimension != m_dimension.id() || (traits && traits->kind >= BetaMobKind::Arrow)) return;
			DoubleBox box = localBox;
			box.move(position.x, position.y, position.z);
			add(box);
		});
}
