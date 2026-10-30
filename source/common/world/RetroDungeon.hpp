#ifndef RETRODUNGEON_HPP_
#define RETRODUNGEON_HPP_

#include <array>
#include <cstdlib>
#include <optional>
#include <vector>

#include "common/core/Vector3.hpp"
#include "common/world/RetroRandom.hpp"

namespace retro {

struct DungeonLoot { const char *id = ""; int count = 0; };
struct DungeonChest { Vector3i position; std::array<DungeonLoot, 27> slots; };
struct DungeonCell { Vector3i position; u8 block; };
struct DungeonRoom {
	Vector3i center;
	int radiusX = 0, radiusY = 0, spawner = 0;
	std::vector<DungeonCell> cells;
	std::vector<DungeonChest> chests;
};

inline bool dungeonSolid(u8 block) { return block && block != 8 && block != 9 && block != 10 && block != 11; }

inline const char *dungeonRareLoot(int roll, int choice) {
	static const char *items[] = {"default:apple", "default:apple_golden", "default:record_13", "default:record_cat",
		"default:record_wait", "default:record_blocks", "default:record_far", "default:record_chirp",
		"default:record_strad", "default:record_mellohi", "default:record_mall"};
	return roll == 0 ? items[unsigned(choice) % 11] : "";
}

inline DungeonLoot dungeonLoot(Random &random) {
	switch (random.nextInt(11)) {
		case 0: return {"default:saddle", 1};
		case 1: return {"default:iron_ingot", random.nextInt(4) + 1};
		case 2: return {"default:bread", 1};
		case 3: return {"default:wheat", random.nextInt(4) + 1};
		case 4: return {"default:gunpowder", random.nextInt(4) + 1};
		case 5: return {"default:string", random.nextInt(4) + 1};
		case 6: return {"default:bucket_empty", 1};
		case 7: random.nextInt(100); break;
		case 8: if (random.nextInt(2) == 0) return {"default:redstone_dust", random.nextInt(4) + 1}; break;
		case 9: if (random.nextInt(10) == 0) random.nextInt(2); break;
		case 10: return {"default:cocoa_beans", 1};
	}
	return {};
}

// worldgendungeons.java, with minecraft's vertical y mapped to this engine's z.
template<typename BaseBlock>
std::optional<DungeonRoom> makeDungeon(Random &random, Vector3i center, BaseBlock base) {
	DungeonRoom room; room.center = center;
	room.radiusX = random.nextInt(2) + 2; room.radiusY = random.nextInt(2) + 2;
	int rx = room.radiusX, ry = room.radiusY, openings = 0;
	if (center.z < 1 || center.z > 123) return std::nullopt;
	u8 blocks[9][9][6]{};
	for (int x = -rx - 1; x <= rx + 1; ++x) for (int z = -1; z <= 4; ++z) for (int y = -ry - 1; y <= ry + 1; ++y) {
		u8 block = base(center.x + x, center.y + y, center.z + z);
		blocks[x + 4][y + 4][z + 1] = block;
		if ((z == -1 || z == 4) && !dungeonSolid(block)) return std::nullopt;
		if ((std::abs(x) == rx + 1 || std::abs(y) == ry + 1) && z == 0 && !block
		 && !base(center.x + x, center.y + y, center.z + 1)) ++openings;
	}
	if (openings < 1 || openings > 5) return std::nullopt;
	for (int x = -rx - 1; x <= rx + 1; ++x) for (int z = 3; z >= -1; --z) for (int y = -ry - 1; y <= ry + 1; ++y) {
		u8 &block = blocks[x + 4][y + 4][z + 1];
		bool write = true;
		if (std::abs(x) != rx + 1 && std::abs(y) != ry + 1 && z != -1) block = 0;
		else if (!dungeonSolid(z == -1 ? base(center.x + x, center.y + y, center.z - 2) : blocks[x + 4][y + 4][z])) block = 0;
		else if (dungeonSolid(block)) block = z == -1 && random.nextInt(4) != 0 ? 48 : 4;
		else write = false;
		if (write) room.cells.push_back({center + Vector3i{x, y, z}, block});
	}
	for (int chest = 0; chest < 2; ++chest) {
		for (int attempt = 0; attempt < 3; ++attempt) {
			int x = random.nextInt(rx * 2 + 1) - rx, y = random.nextInt(ry * 2 + 1) - ry;
			if (blocks[x + 4][y + 4][1]) continue;
			int walls = dungeonSolid(blocks[x + 3][y + 4][1]) + dungeonSolid(blocks[x + 5][y + 4][1])
				+ dungeonSolid(blocks[x + 4][y + 3][1]) + dungeonSolid(blocks[x + 4][y + 5][1]);
			if (walls != 1) continue;
			DungeonChest contents; contents.position = center + Vector3i{x, y, 0};
			for (int roll = 0; roll < 8; ++roll) {
				DungeonLoot loot = dungeonLoot(random);
				if (loot.count) contents.slots[random.nextInt(27)] = loot;
			}
			// one shared rare roll per chest, not one roll per item or loot slot.
			if (random.nextInt(1000000) == 0) contents.slots[random.nextInt(27)] = {dungeonRareLoot(0, random.nextInt(11)), 1};
			blocks[x + 4][y + 4][1] = 54;
			room.cells.push_back({contents.position, 54}); room.chests.push_back(contents);
			break;
		}
	}
	room.spawner = random.nextInt(4);
	room.cells.push_back({center, 52});
	return room;
}

}

#endif
