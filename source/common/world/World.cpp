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
#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "common/core/EngineConfig.hpp"
#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/world/World.hpp"

bool World::isReloadRequested = false;

void World::update() {
	using Clock = std::chrono::steady_clock;
	auto deadline = Clock::now() + std::chrono::milliseconds(2);
	std::size_t updates = std::min<std::size_t>(16, m_chunkUpdateQueue.size());
	while (updates > 0 && !m_chunkUpdateQueue.empty() && Clock::now() < deadline) {
		--updates;
		Chunk *chunk = m_chunkUpdateQueue.front();
		m_chunkUpdateQueue.pop_front();
		m_chunksToUpdate.erase(chunk);
		chunk->update();
	}
	deadline = Clock::now() + std::chrono::milliseconds(2);
	std::size_t processes = std::min<std::size_t>(2, m_chunkProcessQueue.size());
	while (processes > 0 && !m_chunkProcessQueue.empty() && Clock::now() < deadline) {
		--processes;
		Chunk *chunk = m_chunkProcessQueue.front();
		m_chunkProcessQueue.pop_front();
		m_chunksToProcess.erase(chunk);
		chunk->process();
	}
}

void World::removeChunkFromQueues(Chunk *chunk) {
	m_chunksToUpdate.erase(chunk);
	m_chunksToProcess.erase(chunk);
	m_chunkUpdateQueue.erase(std::remove(m_chunkUpdateQueue.begin(), m_chunkUpdateQueue.end(), chunk), m_chunkUpdateQueue.end());
	m_chunkProcessQueue.erase(std::remove(m_chunkProcessQueue.begin(), m_chunkProcessQueue.end(), chunk), m_chunkProcessQueue.end());
}

Chunk *World::getChunkAtBlockPos(int x, int y, int z) const {
	return getChunk(
		(x & -CHUNK_WIDTH)  / CHUNK_WIDTH,
		(y & -CHUNK_DEPTH)  / CHUNK_DEPTH,
		(z & -CHUNK_HEIGHT) / CHUNK_HEIGHT
	);
}

BlockData *World::getBlockData(int x, int y, int z) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		return chunk->getBlockData(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1));

	return nullptr;
}

BlockData *World::addBlockData(int x, int y, int z, u16 inventoryWidth, u16 inventoryHeight) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		return chunk->addBlockData(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1), inventoryWidth, inventoryHeight);

	return nullptr;
}

const BlockState *World::getBlockState(int x, int y, int z) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		return chunk->getBlockState(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1));

	return nullptr;
}

void World::setBlockState(int x, int y, int z, u16 stateID) {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		chunk->setBlockState(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1), stateID);
}


u16 World::getBlock(int x, int y, int z) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		return chunk->getBlock(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1));

	return 0;
}

int World::getLight(int x, int y, int z) const {
	const Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (!chunk || !chunk->isInitialized()) return -1;
	u8 light = chunk->lightmap().getLightDataRaw(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1));
	int nightReduction = int(std::round(11.f * (1.f - GameTime::getSunlightIntensityFromTime(GameTime::getCurrentTime()))));
	return std::max(int(light & 15), std::max(0, int(light >> 4) - nightReduction));
}

void World::setBlock(int x, int y, int z, u16 id) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		chunk->setBlock(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1), id);
}

u16 World::getData(int x, int y, int z) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		return chunk->getData(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1));

	return 0;
}

void World::setData(int x, int y, int z, u16 data) const {
	Chunk *chunk = getChunkAtBlockPos(x, y, z);
	if (chunk)
		chunk->setData(x & (CHUNK_WIDTH - 1), y & (CHUNK_DEPTH - 1), z & (CHUNK_HEIGHT - 1), data);
}

bool World::addChunkToUpdate(Chunk *chunk) {
	if (m_chunksToUpdate.find(chunk) == m_chunksToUpdate.end()) {
		m_chunksToUpdate.emplace(chunk);
		m_chunkUpdateQueue.push_back(chunk);
		return true;
	}
	return false;
}

bool World::addChunkToProcess(Chunk *chunk) {
	if (m_chunksToProcess.find(chunk) == m_chunksToProcess.end()) {
		m_chunksToProcess.emplace(chunk);
		m_chunkProcessQueue.push_back(chunk);
		return true;
	}
	return false;
}

void World::clearUpdateQueues() {
	m_chunkUpdateQueue.clear();
	m_chunkProcessQueue.clear();
	m_chunksToUpdate.clear();
	m_chunksToProcess.clear();
}

// Please update 'docs/lua-api-cpp.md' if you change this
void World::initUsertype(sol::state &lua) {
	lua.new_usertype<World>("World",
		"get_block", &World::getBlock,
		"set_block", &World::setBlock,

		"get_data", &World::getData,
		"set_data", &World::setData,

		"get_block_data", &World::getBlockData,
		"add_block_data", &World::addBlockData,

		"get_block_def", [&](World &self, int x, int y, int z) {
			return Registry::getInstance().getBlock(self.getBlock(x, y, z));
		},

		"set_block_from_str", [&](World &self, int x, int y, int z, const std::string &id) {
			self.setBlock(x, y, z, Registry::getInstance().getBlockFromStringID(id).id());
		},

		"get_block_state", &World::getBlockState,
		"set_block_state", &World::setBlockState,
		"get_light", &World::getLight
	);
}

