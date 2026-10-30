#include <algorithm>
#include "common/core/Registry.hpp"
#include "common/world/Heightmap.hpp"
#include "common/world/ModernMineshaft.hpp"
#include "server/world/OceanTerrain.hpp"
#include "server/world/ServerChunk.hpp"

void decorateOceanTerrain(ServerChunk &chunk, Heightmap &heightmap, s32 seed) {
	if (chunk.isInitialized() || chunk.z() < 1 || chunk.z() > 2) return;
	auto &registry = Registry::getInstance();
	if (!registry.hasBlock("default:tube_coral_block")) return;
	const auto &base = heightmap.getOrCreateChunk(chunk.x(), chunk.y());
	int ox = chunk.x() * CHUNK_WIDTH, oy = chunk.y() * CHUNK_DEPTH, oz = chunk.z() * CHUNK_HEIGHT;
	static constexpr const char *species[] = {"tube", "brain", "bubble", "fire", "horn"};
	for (int y = 0; y < CHUNK_DEPTH; ++y) for (int x = 0; x < CHUNK_WIDTH; ++x) {
		auto biome = base.biomeAt(x, y);
		if (biome == retro::Biome::Taiga || biome == retro::Biome::Tundra || biome == retro::Biome::IceDesert) continue;
		int floor = 62;
		while (floor > 32 && (base.blockAt(x, y, floor) == 9 || base.blockAt(x, y, floor) == 0)) --floor;
		if (floor < 40 || floor > 59 || base.blockAt(x, y, floor + 1) != 9) continue;
		int wx = ox + x, wy = oy + y;
		auto patch = modern::Mineshaft::originRandom(seed ^ 0x434f5241, (wx & -32) / 32, (wy & -32) / 32);
		if (patch.nextInt(3)) continue;
		int centerX = (wx & -32) + 8 + patch.nextInt(16), centerY = (wy & -32) + 8 + patch.nextInt(16);
		int radius = 4 + patch.nextInt(4), kind = patch.nextInt(5);
		int distance = (wx - centerX) * (wx - centerX) + (wy - centerY) * (wy - centerY);
		if (distance > radius * radius) continue;
		auto random = modern::Mineshaft::originRandom(seed ^ 0x52454546, wx, wy);
		if (random.nextInt(5) == 0) continue;
		std::string name = std::string("default:") + species[kind] + "_coral";
		int height = 1 + (distance < radius * radius / 2 ? random.nextInt(3) : 0);
		u16 block = registry.getBlockFromStringID(name + "_block").id();
		for (int dz = 1; dz <= height; ++dz) {
			int z = floor + dz - oz;
			if (z >= 0 && z < CHUNK_HEIGHT && base.blockAt(x, y, floor + dz) == 9) chunk.setBlockRaw(x, y, z, block);
		}
		int plantZ = floor + height + 1;
		if (plantZ < 62 && plantZ >= oz && plantZ < oz + CHUNK_HEIGHT && random.nextInt(3) != 0) {
			u16 plant = registry.getBlockFromStringID(name + (random.nextInt(2) ? "_fan" : "")).id();
			chunk.setBlockRaw(x, y, plantZ - oz, plant);
		}
	}
}

void decorateNetherRuins(ServerChunk &chunk, Heightmap &heightmap, s32 seed) {
	if (chunk.isInitialized() || chunk.z() < 1 || chunk.z() > 3) return;
	auto &registry = Registry::getInstance();
	if (!registry.hasBlock("default:nether_bricks")) return;
	int ox = chunk.x() * CHUNK_WIDTH, oy = chunk.y() * CHUNK_DEPTH, oz = chunk.z() * CHUNK_HEIGHT;
	// small ruined halls sit on exposed netherrack shelves, clear of the lava sea.
	int cellX = (ox & -64) / 64, cellY = (oy & -64) / 64;
	auto random = modern::Mineshaft::originRandom(seed ^ 0x4e425249, cellX, cellY);
	if (random.nextInt(3)) return;
	int cx = cellX * 64 + 16 + random.nextInt(32), cy = cellY * 64 + 16 + random.nextInt(32);
	auto read = [&](int x, int y, int z) { return heightmap.getOrCreateChunk((x & -16) / 16, (y & -16) / 16).blockAt(x & 15, y & 15, z); };
	int floor = 80;
	for (; floor >= 35; --floor) if (read(cx, cy, floor) == 87 && !read(cx, cy, floor + 1) && !read(cx, cy, floor + 5)) break;
	if (floor < 35) return;
	u16 brick = registry.getBlockFromStringID("default:nether_bricks").id();
	u16 cracked = registry.getBlockFromStringID("default:cracked_nether_bricks").id();
	for (int y = 0; y < CHUNK_DEPTH; ++y) for (int x = 0; x < CHUNK_WIDTH; ++x) {
		int dx = ox + x - cx, dy = oy + y - cy;
		if (std::abs(dx) > 5 || std::abs(dy) > 6) continue;
		for (int h = 0; h < 6; ++h) {
			int z = floor + h - oz;
			if (z < 0 || z >= CHUNK_HEIGHT) continue;
			bool pillar = std::abs(dx) == 5 && (dy + 6) % 4 == 0;
			bool wall = std::abs(dx) == 5 || (std::abs(dy) == 6 && std::abs(dx) > 1);
			auto detail = modern::Mineshaft::originRandom(seed ^ h, ox + x, oy + y);
			bool solid = h == 0 || pillar || (wall && h <= 3 && detail.nextInt(4));
			chunk.setBlockRaw(x, y, z, solid ? (detail.nextInt(4) ? brick : cracked) : 0);
		}
	}
}
