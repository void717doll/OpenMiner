#include <cstdint>

#include "common/core/Registry.hpp"
#include "common/world/Heightmap.hpp"
#include "server/world/LushCaves.hpp"
#include "server/world/ServerChunk.hpp"

namespace {

std::uint64_t mix(std::uint64_t value) {
	value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
	value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
	return value ^ (value >> 31);
}

std::uint64_t featureHash(s32 seed, int x, int y, int z, std::uint64_t salt) {
	std::uint64_t value = mix(std::uint32_t(seed) ^ salt);
	value = mix(value ^ std::uint64_t(std::uint32_t(x)) * 0x9e3779b185ebca87ULL);
	value = mix(value ^ std::uint64_t(std::uint32_t(y)) * 0xc2b2ae3d27d4eb4fULL);
	return mix(value ^ std::uint64_t(std::uint32_t(z)) * 0x165667b19e3779f9ULL);
}

int cellAt(int coordinate) {
	int result = coordinate / 16;
	return coordinate % 16 < 0 ? result - 1 : result;
}

bool lushPatch(s32 seed, int x, int y, int z) {
	for (int cz = cellAt(z) - 1; cz <= cellAt(z) + 1; ++cz) {
		for (int cy = cellAt(y) - 1; cy <= cellAt(y) + 1; ++cy) {
			for (int cx = cellAt(x) - 1; cx <= cellAt(x) + 1; ++cx) {
				auto hash = featureHash(seed, cx, cy, cz, 0x4c55534850415443ULL);
				if (hash % 5 != 0) continue;
				int dx = x - (cx * 16 + int((hash >> 8) & 15));
				int dy = y - (cy * 16 + int((hash >> 16) & 15));
				int dz = z - (cz * 16 + int((hash >> 24) & 15));
				int radius = 5 + int((hash >> 32) % 3);
				if (dx * dx + dy * dy + dz * dz * 2 <= radius * radius) return true;
			}
		}
	}
	return false;
}

u16 blockID(const char *name) {
	auto &registry = Registry::getInstance();
	return registry.hasBlock(name) ? registry.getBlockFromStringID(name).id() : 0;
}

void place(ServerChunk &chunk, int x, int y, int z, u16 id, bool berries = false) {
	if (!id) return;
	chunk.setBlockRaw(x, y, z, id);
	if (berries) chunk.setBlockState(x, y, z, 1);
}

bool supportInChunk(const ServerChunk &chunk, int x, int y, int z) {
	if (z < 0 || z >= CHUNK_HEIGHT) return true;
	return Registry::getInstance().getBlock(chunk.getBlock(x, y, z)).getState(0).isCollidable();
}

}

void decorateLushCaves(ServerChunk &chunk, Heightmap &heightmap, s32 seed) {
	if (heightmap.isNether() || chunk.z() < 0 || chunk.z() >= 2) return;
	u16 moss = blockID("default:moss_block"), rooted = blockID("default:rooted_dirt");
	if (!moss || !rooted) return;
	u16 stone = blockID("default:stone"), dirt = blockID("default:dirt");
	u16 carpet = blockID("default:moss_carpet"), roots = blockID("default:hanging_roots");
	u16 vines = blockID("default:cave_vines"), blossom = blockID("default:spore_blossom");
	u16 azalea = blockID("default:azalea"), flowering = blockID("default:flowering_azalea");
	auto &base = heightmap.getOrCreateChunk(chunk.x(), chunk.y());
	int originX = chunk.x() * CHUNK_WIDTH, originY = chunk.y() * CHUNK_DEPTH;
	int originZ = chunk.z() * CHUNK_HEIGHT;
	for (int z = 0; z < CHUNK_HEIGHT; ++z) {
		int worldZ = originZ + z;
		if (worldZ < 16 || worldZ > 52) continue;
		for (int y = 0; y < CHUNK_DEPTH; ++y) {
			for (int x = 0; x < CHUNK_WIDTH; ++x) {
				if (base.landHeightAt(x, y) - worldZ < 12) continue;
				int worldX = originX + x, worldY = originY + y;
				int kind = base.blockAt(x, y, worldZ);
				u16 current = chunk.getBlock(x, y, z);
				if ((kind == 1 || kind == 3) && current && (current == stone || current == dirt)) {
					if (!lushPatch(seed, worldX, worldY, worldZ)) continue;
					if (base.blockAt(x, y, worldZ + 1) == 0)
						place(chunk, x, y, z, moss);
					else if (base.blockAt(x, y, worldZ - 1) == 0) {
						auto detail = featureHash(seed, worldX, worldY, worldZ, 0x4c5553484345494cULL) % 16;
						if (detail >= 1 && detail <= 4) place(chunk, x, y, z, rooted);
					}
					continue;
				}
				if (kind != 0 || current) continue;
				int floor = base.blockAt(x, y, worldZ - 1);
				if ((floor == 1 || floor == 3) && supportInChunk(chunk, x, y, z - 1)
					&& lushPatch(seed, worldX, worldY, worldZ - 1)) {
					auto detail = featureHash(seed, worldX, worldY, worldZ, 0x4c555348464c4f52ULL) % 24;
					if (detail < 4) place(chunk, x, y, z, carpet);
					else if (z == 0 || chunk.getBlock(x, y, z - 1) == moss) {
						if (detail == 4) place(chunk, x, y, z, azalea);
						else if (detail == 5) place(chunk, x, y, z, flowering);
					}
					if (chunk.getBlock(x, y, z)) continue;
				}
				// each segment replays its ceiling anchor, including across vertical chunk borders.
				for (int distance = 1; distance <= 3; ++distance) {
					int ceiling = worldZ + distance, ceilingKind = base.blockAt(x, y, ceiling);
					if (ceilingKind == 0) continue;
					if ((ceilingKind != 1 && ceilingKind != 3) || ceiling > 52 || base.landHeightAt(x, y) - ceiling < 12
						|| !supportInChunk(chunk, x, y, z + distance) || !lushPatch(seed, worldX, worldY, ceiling)) break;
					auto hash = featureHash(seed, worldX, worldY, ceiling, 0x4c5553484345494cULL);
					unsigned detail = unsigned(hash % 16);
					if (detail == 0 && distance == 1) place(chunk, x, y, z, blossom);
					else if (detail >= 1 && detail <= 4 && distance == 1) place(chunk, x, y, z, roots);
					else if (detail >= 5 && detail <= 10 && vines) {
						int length = 1 + int((hash >> 8) % 3);
						for (int part = 1; part <= length; ++part) {
							if (base.blockAt(x, y, ceiling - part) != 0) { length = part - 1; break; }
						}
						bool clear = distance <= length;
						for (int part = 1; clear && part < distance; ++part) {
							int localZ = z + distance - part;
							if (localZ >= CHUNK_HEIGHT) continue;
							u16 occupied = chunk.getBlock(x, y, localZ);
							if (occupied && occupied != vines) clear = false;
						}
						if (clear) place(chunk, x, y, z, vines, distance == length && (hash >> 16) % 7 == 0);
					}
					break;
				}
			}
		}
	}
}
