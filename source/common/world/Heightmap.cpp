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
#include "common/world/Heightmap.hpp"
#include "common/world/RetroCaves.hpp"
#include "common/world/World.hpp"

void HeightmapChunk::generate() {
	if (m_heightmap.isNether()) {
		m_density = m_heightmap.netherTerrain().density(m_x, m_y);
		m_blocks = m_heightmap.netherTerrain().surface(m_x, m_y, m_density);
	}
	else {
		m_climate = m_heightmap.terrain().climate().sample(m_x * 16, m_y * 16);
		m_density = m_heightmap.terrain().density(m_x, m_y, m_climate);
		m_blocks = m_heightmap.terrain().surface(m_x, m_y, m_climate, m_density);
	}
	retro::Caves::carve(m_heightmap.terrain().seed(), m_x, m_y, m_blocks, m_heightmap.isNether());
	refreshHeight();
}

void HeightmapChunk::refreshHeight() {
	for (int y = 0; y < CHUNK_DEPTH; ++y) {
		for (int x = 0; x < CHUNK_WIDTH; ++x) {
			int height = 127;
			while (height > 0 && (blockAt(x, y, height) == 0 || blockAt(x, y, height) == 9)) --height;
			m_map[x + y * CHUNK_WIDTH] = height;
		}
	}
}

s32 HeightmapChunk::landHeightAt(s8 x, s8 y) const {
	return m_map[x + y * CHUNK_WIDTH];
}

void HeightmapChunk::setLandHeight(s8 x, s8 y, s32 height) {
	m_map[x + y * CHUNK_WIDTH] = height;
}

Heightmap::Heightmap(s32 seed, bool nether) : m_terrain(seed), m_netherTerrain(seed), m_nether(nether) {}

HeightmapChunk &Heightmap::getOrCreateChunk(s32 chunkX, s32 chunkY) {
	HeightmapChunk *chunk = nullptr;

	auto it = m_chunks.find({chunkX, chunkY});
	if (it == m_chunks.end()) {
		m_chunks.emplace(Vector2i{chunkX, chunkY}, HeightmapChunk{*this, chunkX, chunkY});

		chunk = &m_chunks.at({chunkX, chunkY});
		chunk->generate();
	}
	else
		chunk = &it->second;

	return *chunk;
}

int Heightmap::getHighestBlockAt(s32 blockX, s32 blockY) {
	s32 chunkX = (blockX & -CHUNK_WIDTH) / CHUNK_WIDTH;
	s32 chunkY = (blockY & -CHUNK_DEPTH) / CHUNK_DEPTH;

	s32 blockZ = getOrCreateChunk(chunkX, chunkY).landHeightAt(
		s8(math::pmod(blockX, CHUNK_WIDTH)),
		s8(math::pmod(blockY, CHUNK_DEPTH))
	);

	return blockZ;
}

int Heightmap::getHighestChunkAt(s32 blockX, s32 blockY) {
	return (getHighestBlockAt(blockX, blockY) & -CHUNK_HEIGHT) / CHUNK_HEIGHT;
}

void Heightmap::setSeed(s32 seed) {
	m_chunks.clear();
	m_terrain = retro::Terrain(seed);
	m_netherTerrain = retro::NetherTerrain(seed);
}
