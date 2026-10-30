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
#include "common/core/Registry.hpp"
#include "common/world/World.hpp"
#include "common/world/Dimension.hpp"
#include "server/world/ServerChunk.hpp"
#include "server/world/TerrainGenerator.hpp"

#include <array>

TerrainGenerator::TerrainGenerator(Heightmap &heightmap, const Dimension &dimension, s32 seed)
	: m_biomeSampler(dimension, seed), m_heightmap(heightmap), m_dimensionID(dimension.id())
{
	setSeed(seed);
}

void TerrainGenerator::generate(ServerChunk &chunk) {
	fastNoiseGeneration(chunk);
}

void TerrainGenerator::fastNoiseGeneration(ServerChunk &chunk) {
	HeightmapChunk &heightmap = m_heightmap.getOrCreateChunk(chunk.x(), chunk.y());
	if (m_dimensionID == 1) heightmap.populateNether();

	Random_t rand;
	std::seed_seq seed{u32(m_seed), u32(m_dimensionID), u32(chunk.x()), u32(chunk.y()), u32(chunk.z())};
	std::array<u32, 1> chunkSeed;
	seed.generate(chunkSeed.begin(), chunkSeed.end());
	rand.seed(chunkSeed[0]);
	Random_t copperRandom;
	copperRandom.seed(chunkSeed[0] ^ 0x43555052u);
	auto &definitions = Registry::getInstance();
	u16 copper = m_dimensionID == 0 && definitions.hasBlock("default:copper_ore")
		? definitions.getBlockFromStringID("default:copper_ore").id() : 0;

	for(s8 y = 0 ; y < CHUNK_DEPTH ; y++) {
		for(s8 x = 0 ; x < CHUNK_WIDTH ; x++) {
			u16 biomeIndex = m_dimensionID == 0 ? m_biomeSampler.getBetaBiomeIndex(heightmap.biomeAt(x, y))
				: m_biomeSampler.getBiomeIndexAt(x + chunk.x() * CHUNK_WIDTH, y + chunk.y() * CHUNK_DEPTH);
			const Biome &biome = Registry::getInstance().getBiome(biomeIndex);

			// Land height
			double h = heightmap.landHeightAt(x, y);

			for (s8 z = 0; z < CHUNK_HEIGHT; ++z) {
				int worldZ = z + chunk.z() * CHUNK_HEIGHT;
				bool isGeneratingTopBlock = worldZ == h && worldZ >= SEALEVEL;
				if (m_dimensionID == 0 || m_dimensionID == 1) {
					u8 kind = heightmap.blockAt(x, y, worldZ);
					u16 block = 0;
					const char *id = nullptr;
					switch (kind) {
						case 1: block = biome.getDeepBlockID(); break;
						case 2: block = biome.getTopBlockID(); break;
						case 3: block = biome.getGroundBlockID(); break;
						case 7: id = "default:bedrock"; break;
						case 8: case 9: block = biome.getLiquidBlockID(); break;
						case 10: case 11: id = "default:lava"; break;
						case 12: id = "default:sand"; break;
						case 13: id = "default:gravel"; break;
						case 24: id = "default:sandstone"; break;
						case 79: id = "default:ice"; break;
						case 87: id = "default:netherrack"; break;
						case 88: id = "default:soul_sand"; break;
						case 89: id = "default:glowstone"; break;
					}
					if (id) {
						auto &registry = Registry::getInstance();
						block = registry.hasBlock(id) ? registry.getBlockFromStringID(id).id() : biome.getDeepBlockID();
					}
					// preserve decorations previously written by neighboring chunks.
					if (block || !chunk.getBlock(x, y, z)) chunk.setBlockRaw(x, y, z, block);
				}
				else if (heightmap.densityAt(x, y, worldZ) <= 0) {
					if (worldZ < SEALEVEL) chunk.setBlockRaw(x, y, z, biome.getLiquidBlockID());
				}
				else {
					if (isGeneratingTopBlock) chunk.setBlockRaw(x, y, z, biome.getTopBlockID());
					else if (worldZ <= SEALEVEL - 1 && h < SEALEVEL && worldZ > h - 3)
						chunk.setBlockRaw(x, y, z, biome.getBeachBlockID());
					else if (worldZ > h - 3) chunk.setBlockRaw(x, y, z, biome.getGroundBlockID());
					else chunk.setBlockRaw(x, y, z, biome.getDeepBlockID());
				}
				if (chunk.getBlock(x, y, z) == biome.getDeepBlockID()) generateOres(chunk, x, y, z, biome, rand);
				if (copper && worldZ >= 4 && worldZ <= 64 && chunk.getBlock(x, y, z) == biome.getDeepBlockID()
				 && copperRandom.get<bool>(0.004f))
					randomWalkOrePlace(chunk, x, y, z, copperRandom, copper, biome.getDeepBlockID(), 8);
				if (isGeneratingTopBlock && chunk.getBlock(x, y, z) == biome.getTopBlockID()) {
					bool placedTree = tryPlaceTree(chunk, x, y, z + 1, biome, rand);
					bool placedFlora = !placedTree && tryPlaceFlora(chunk, x, y, z + 1, biome, rand);
					if (!placedTree && !placedFlora) tryPlacePortal(chunk, x, y, z + 1, biome, rand);
					if (m_dimensionID == 0 && heightmap.temperatureAt(x, y) < 0.5 && !chunk.getBlock(x, y, z + 1)) {
						auto &registry = Registry::getInstance();
						if (registry.hasBlock("default:snow")) chunk.setBlockRaw(x, y, z + 1, registry.getBlockFromStringID("default:snow").id());
					}
				}

			}
		}
	}
}

inline bool TerrainGenerator::tryPlaceTree(ServerChunk &chunk, int x, int y, int z, const Biome &biome, Random_t &rand) const {
	if (chunk.getBlock(x, y, z - 1) == biome.getTopBlockID()) {
		for (const PlacementEntry::Tree &treePlacement : biome.getTrees()) {
			if (!rand.get<bool>(treePlacement.probability))
				continue;

			const Tree &tree = Registry::getInstance().getTree(treePlacement.treeID);

			// Trunk
			int h = rand.get(tree.trunkMinHeight(), tree.trunkMaxHeight());
			for (int i = 0; i < h; i++) {
				chunk.setBlockRaw(x, y, z + i, tree.getLogBlockID());
			}

			// Leaves
			if (tree.hasLeaves()) {
				for (int iz = -3; iz <= 3; iz++) {
					for (int iy = -3; iy <= 3; iy++) {
						for (int ix = -3; ix <= 3; ix++) {
							u16 block = chunk.getBlock(x + ix, y + iy, z + h + iz);
							if (ix * ix + iy * iy + iz * iz < 8 + rand.get(0, 1) && !block) {
								chunk.setBlockRaw(x + ix, y + iy, z + h + iz, tree.getLeavesBlockID());

								// FIXME: This is a temporary fix for the second part of #41
								chunk.lightmap().setSunlight(x + ix, y + iy, z + h + iz, 0);
							}
						}
					}
				}
			}

			return true;
		}
	}

	return false;
}

inline bool TerrainGenerator::tryPlaceFlora(ServerChunk &chunk, int x, int y, int z, const Biome &biome, Random_t &rand) const {
	for (const PlacementEntry::Flora &flora : biome.getFlora()) {
		if (chunk.getBlock(x, y, z - 1) != flora.spawnsOnBlockID)
			continue;

		if (!rand.get<bool>(flora.probability))
			continue;

		chunk.setBlockRaw(x, y, z, flora.blockID);

		return true;
	}

	return false;
}

inline bool TerrainGenerator::tryPlacePortal(ServerChunk &chunk, int x, int y, int z, const Biome &biome, Random_t &rand) const {
	// FIXME: This is a temporary portal generation
	//        This code should be replaced by a proper "feature" implementation
	//        which will also allow making stuff like villages easier
	if (chunk.getBlock(x, y, z - 1) == biome.getTopBlockID() && rand.get<bool>(0.0002)) {
		for (int ix = 0 ; ix < 4 ; ++ix) {
			for (int iz = 0 ; iz < 5 ; ++iz) {
				if (ix == 0 || iz == 0 || ix == 3 || iz == 4)
					chunk.setBlockRaw(x + ix, y, z + iz, biome.getPortalFrameBlockID());
				else
					chunk.setBlockRaw(x + ix, y, z + iz, biome.getPortalBlockID());
			}
		}

		return true;
	}

	return false;
}

inline void TerrainGenerator::generateOres(ServerChunk &chunk, int x, int y, int z, const Biome &biome, Random_t &rand) const {
	// TODO: Like trees, ores should be able to seamlessly cross chunk boundaries.
	// This could be achieved either by setting up a generation pipeline with stages,
	// processing neighboring chunks' ores every time, or generating them with noise.
	for (const PlacementEntry::Ore &ore : biome.getOres()) {
		if (!rand.get<bool>(ore.probability))
			continue;

		if (ore.genType == PlacementEntry::Ore::Gen::RandomWalk)
			randomWalkOrePlace(chunk, x, y, z, rand, ore.blockID, biome.getDeepBlockID(), ore.size);
		else if (ore.genType == PlacementEntry::Ore::Gen::FloodFill)
			oreFloodFill(chunk, x, y, z, biome.getDeepBlockID(), ore.blockID, ore.size, rand);

		break;
	}
}

inline void TerrainGenerator::randomWalkOrePlace(ServerChunk &chunk, int x, int y, int z, Random_t &rand, u16 oreBlock, u16 deepBlock, int size) const {
	if (size <= 0 || chunk.getBlock(x, y, z) != deepBlock)
		return;
	chunk.setBlockRaw(x, y, z, oreBlock);
	int direction = rand.get(1, 6);
	switch(direction) {
		case 1:
			randomWalkOrePlace(chunk, x + 1, y, z, rand, oreBlock, deepBlock, size - 1);
			break;
		case 2:
			randomWalkOrePlace(chunk, x - 1, y, z, rand, oreBlock, deepBlock, size - 1);
			break;
		case 3:
			randomWalkOrePlace(chunk, x, y + 1, z, rand, oreBlock, deepBlock, size - 1);
			break;
		case 4:
			randomWalkOrePlace(chunk, x, y - 1, z, rand, oreBlock, deepBlock, size - 1);
			break;
		case 5:
			randomWalkOrePlace(chunk, x, y, z + 1, rand, oreBlock, deepBlock, size - 1);
			break;
		case 6:
			randomWalkOrePlace(chunk, x, y, z - 1, rand, oreBlock, deepBlock, size - 1);
			break;
	}
}

void TerrainGenerator::oreFloodFill(ServerChunk &chunk, int x, int y, int z, u16 toReplace, u16 replaceWith, int depth, Random_t &rand) const {
	if (depth < 0) return;
	if (chunk.getBlock(x, y, z) == replaceWith) return;
	if (chunk.getBlock(x, y, z) == toReplace)
		chunk.setBlockRaw(x, y, z, replaceWith);

	oreFloodFill(chunk, x + 1, y, z, toReplace, replaceWith, depth - 1, rand);
	oreFloodFill(chunk, x - 1, y, z, toReplace, replaceWith, depth - 1, rand);
	oreFloodFill(chunk, x, y + 1, z, toReplace, replaceWith, depth - 1, rand);
	oreFloodFill(chunk, x, y - 1, z, toReplace, replaceWith, depth - 1, rand);
	oreFloodFill(chunk, x, y, z + 1, toReplace, replaceWith, depth - 1, rand);
	oreFloodFill(chunk, x, y, z - 1, toReplace, replaceWith, depth - 1, rand);

	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x + 1, y + 1, z + 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x + 1, y + 1, z - 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x + 1, y - 1, z + 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x + 1, y - 1, z - 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x - 1, y + 1, z + 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x - 1, y + 1, z - 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x - 1, y - 1, z + 1, toReplace, replaceWith, depth - 1, rand);
	if (rand.get<bool>(1.f / 15.f))
		oreFloodFill(chunk, x - 1, y - 1, z - 1, toReplace, replaceWith, depth - 1, rand);
}
