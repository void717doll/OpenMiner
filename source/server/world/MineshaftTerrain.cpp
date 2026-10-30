#include <map>

#include "common/core/Registry.hpp"
#include "common/world/Heightmap.hpp"
#include "common/world/ModernMineshaft.hpp"
#include "server/world/MineshaftTerrain.hpp"
#include "server/world/ServerChunk.hpp"

namespace {

struct MineCell {
	u16 block = 0, data = 0;
	int lootSeed = 0;
};

void fillLoot(Inventory &inventory, int seed) {
	struct Loot { const char *id; int min, max, weight; };
	static constexpr Loot table[] = {
		{"default:iron_ingot", 1, 5, 10}, {"default:gold_ingot", 1, 3, 5},
		{"default:redstone_dust", 4, 9, 5}, {"default:lapis_lazuli", 4, 9, 5},
		{"default:diamond", 1, 2, 3}, {"default:coal", 3, 8, 10},
		{"default:bread", 1, 3, 15}, {"default:iron_pickaxe", 1, 1, 1},
		{"default:rail", 4, 8, 1},
	};
	retro::Random random(seed);
	int rolls = 3 + random.nextInt(4);
	for (int i = 0; i < rolls; ++i) {
		int choice = random.nextInt(55);
		for (const auto &loot : table) {
			choice -= loot.weight;
			if (choice >= 0) continue;
			int amount = loot.min + random.nextInt(loot.max - loot.min + 1);
			if (Registry::getInstance().hasItem(loot.id)) inventory.addStack(loot.id, amount);
			break;
		}
	}
}

}

void decorateMineshaftTerrain(ServerChunk &chunk, Heightmap &heightmap, s32 seed, modern::MineshaftCache &cache) {
	if (chunk.isInitialized() || chunk.z() < 0 || chunk.z() > 3) return;
	auto &definitions = Registry::getInstance();
	for (const char *id : {"default:oak_fence", "default:cobweb", "default:chest", "default:mob_spawner", "default:rail"})
		if (!definitions.hasBlock(id)) return;
	auto id = [&](const char *name) { return definitions.getBlockFromStringID(name).id(); };
	const u16 planks = id("default:oak_planks"), fence = id("default:oak_fence"), web = id("default:cobweb");
	const u16 dirt = id("default:dirt"), chest = id("default:chest"), spawner = id("default:mob_spawner");
	const auto &rail = definitions.getBlockFromStringID("default:rail");
	const auto &torch = definitions.getBlockFromStringID("default:torch");
	const int ox = chunk.x() * CHUNK_WIDTH, oy = chunk.y() * CHUNK_DEPTH, oz = chunk.z() * CHUNK_HEIGHT;
	const modern::MineBox bounds{ox, oy, oz, ox + CHUNK_WIDTH - 1, oy + CHUNK_DEPTH - 1, oz + CHUNK_HEIGHT - 1};
	auto base = [&](int x, int y, int z) {
		return heightmap.getOrCreateChunk((x & -16) / 16, (y & -16) / 16).blockAt(x & 15, y & 15, z);
	};
	std::map<int, MineCell> cells;
	auto put = [&](int x, int y, int z, u16 block, u16 data = 0, int lootSeed = 0) {
		if (x < bounds.x0 || x > bounds.x1 || y < bounds.y0 || y > bounds.y1 || z < bounds.z0 || z > bounds.z1) return;
		cells[((z - oz) * CHUNK_DEPTH + y - oy) * CHUNK_WIDTH + x - ox] = {block, data, lootSeed};
	};
	auto fill = [&](const modern::MineBox &box, u16 block) {
		for (int z = box.z0; z <= box.z1; ++z) for (int y = box.y0; y <= box.y1; ++y)
			for (int x = box.x0; x <= box.x1; ++x) put(x, y, z, block);
	};
	// replay whole pieces, then clip writes; chunk load order must not change the structure.
	for (int cx = chunk.x() - 8; cx <= chunk.x() + 8; ++cx) for (int cy = chunk.y() - 8; cy <= chunk.y() + 8; ++cy) {
		const auto &layout = cache.get(seed, cx, cy);
		for (unsigned index = 0; index < layout.pieces.size(); ++index) {
			const auto &piece = layout.pieces[index];
			const auto b = piece.box;
			auto reach = b;
			--reach.z0;
			if (!reach.intersects(bounds)) continue;
			bool wet = false;
			for (int z = b.z0 - 1; z <= b.z1 + 1 && !wet; ++z) for (int y = b.y0 - 1; y <= b.y1 + 1 && !wet; ++y)
				for (int x = b.x0 - 1; x <= b.x1 + 1; ++x) {
					if (x != b.x0 - 1 && x != b.x1 + 1 && y != b.y0 - 1 && y != b.y1 + 1 && z != b.z0 - 1 && z != b.z1 + 1) continue;
					int block = base(x, y, z);
					if (block >= 8 && block <= 11) { wet = true; break; }
				}
			if (wet) continue;
			auto random = modern::Mineshaft::originRandom(seed, cx, cy);
			random.setSeed(random.nextLong() ^ std::int64_t(index * 341873128712ULL));
			auto local = [&](int x, int y, int z) {
				if (piece.direction == 1) return Vector3i{b.x1 - y, b.y0 + x, b.z0 + z};
				if (piece.direction == 2) return Vector3i{b.x0 + x, b.y1 - y, b.z0 + z};
				if (piece.direction == 3) return Vector3i{b.x0 + y, b.y0 + x, b.z0 + z};
				return Vector3i{b.x0 + x, b.y0 + y, b.z0 + z};
			};
			auto place = [&](int x, int y, int z, u16 block, u16 data = 0, int lootSeed = 0) {
				auto p = local(x, y, z); put(p.x, p.y, p.z, block, data, lootSeed);
			};
			if (piece.kind == modern::MinePiece::Room) {
				for (int y = b.y0; y <= b.y1; ++y) for (int x = b.x0; x <= b.x1; ++x)
					if (base(x, y, b.z0)) put(x, y, b.z0, dirt);
				fill({b.x0, b.y0, b.z0 + 1, b.x1, b.y1, b.z0 + 3}, 0);
				for (auto door : piece.entrances) { door.z0 = door.z1 - 2; fill(door, 0); }
				float width = b.x1 - b.x0 + 1, depth = b.y1 - b.y0 + 1;
				for (int z = b.z0 + 4; z <= b.z1; ++z) for (int y = b.y0; y <= b.y1; ++y) for (int x = b.x0; x <= b.x1; ++x) {
					float nx = (x - (b.x0 + width / 2)) / (width / 2), ny = (y - (b.y0 + depth / 2)) / (depth / 2);
					float nz = float(z - b.z0 - 4) / float(b.z1 - b.z0 - 3);
					if (nx * nx + ny * ny + nz * nz <= 1.05f) put(x, y, z, 0);
				}
				continue;
			}
			if (piece.kind == modern::MinePiece::Crossing) {
				for (int z = b.z0; z <= b.z1; ++z) {
					if (z == b.z0 + 3 && b.z1 - b.z0 > 2) fill({b.x0 + 1, b.y0 + 1, z, b.x1 - 1, b.y1 - 1, z}, 0);
					else {
						fill({b.x0 + 1, b.y0, z, b.x1 - 1, b.y1, z}, 0);
						fill({b.x0, b.y0 + 1, z, b.x1, b.y1 - 1, z}, 0);
					}
					for (int x : {b.x0 + 1, b.x1 - 1}) for (int y : {b.y0 + 1, b.y1 - 1}) put(x, y, z, planks);
				}
				for (int y = b.y0; y <= b.y1; ++y) for (int x = b.x0; x <= b.x1; ++x)
					if (!base(x, y, b.z0 - 1)) put(x, y, b.z0 - 1, planks);
				continue;
			}
			if (piece.kind == modern::MinePiece::Stairs) {
				for (int y = 0; y < 9; ++y) {
					int bottom = y < 2 ? 5 : y >= 7 ? 0 : 7 - y - (y < 6 ? 1 : 0);
					int top = y < 2 ? 7 : y >= 7 ? 2 : 9 - y;
					for (int z = bottom; z <= top; ++z) for (int x = 0; x < 3; ++x) place(x, y, z, 0);
				}
				continue;
			}
			int length = piece.direction % 2 ? b.x1 - b.x0 + 1 : b.y1 - b.y0 + 1;
			for (int y = 0; y < length; ++y) for (int x = 0; x < 3; ++x) {
				place(x, y, 0, 0); place(x, y, 1, 0);
				if (random.nextFloat() < 0.8f) place(x, y, 2, 0);
				auto floor = local(x, y, -1);
				if (!base(floor.x, floor.y, floor.z)) place(x, y, -1, planks);
			}
			if (piece.spiders) for (int y = 0; y < length; ++y) for (int z = 0; z < 2; ++z) for (int x = 0; x < 3; ++x)
				if (random.nextFloat() < 0.6f) place(x, y, z, web);
			bool spawned = false;
			for (int support = 2; support < length; support += 5) {
				for (int x : {0, 2}) for (int z = 0; z < 2; ++z) place(x, support, z, fence);
				bool beam = random.nextInt(4) != 0;
				for (int x = 0; x < 3; ++x) if (x != 1 || beam) place(x, support, 2, planks);
				for (int distance : {1, -1, 2, -2}) for (int x : {0, 2})
					if (random.nextFloat() < (std::abs(distance) == 1 ? 0.1f : 0.05f)) place(x, support + distance, 2, web);
				for (int side : {-1, 1}) {
					if (random.nextFloat() >= 0.05f || !beam) continue;
					auto anchor = local(1, support, 2), position = local(1, support + side, 2);
					int rotation = anchor.x < position.x ? 1 : anchor.x > position.x ? 3 : anchor.y < position.y ? 2 : 4;
					place(1, support + side, 2, torch.id(), torch.param().setParam(BlockParam::Rotation, 0, rotation));
				}
				for (int side : {-1, 1}) if (random.nextInt(100) == 0) {
					int lootSeed = int(random.next(31));
					place(side == -1 ? 2 : 0, support + side, 0, chest, 0, lootSeed);
				}
				if (piece.spiders && !spawned) {
					place(1, support - 1 + random.nextInt(3), 0, spawner, 0, int(random.next(31)));
					spawned = true;
				}
			}
			if (piece.rails) for (int y = 0; y < length; ++y) if (random.nextFloat() < 0.7f)
				place(1, y, 0, rail.id(), rail.param().setParam(BlockParam::State, 0, piece.direction % 2));
		}
	}
	for (const auto &entry : cells) {
		int x = entry.first % CHUNK_WIDTH, y = entry.first / CHUNK_WIDTH % CHUNK_DEPTH, z = entry.first / (CHUNK_WIDTH * CHUNK_DEPTH);
		if (chunk.getBlockData(x, y, z)) continue;
		const auto &cell = entry.second;
		chunk.setBlockRaw(x, y, z, cell.block); chunk.setData(x, y, z, cell.data);
		if (cell.block == chest) {
			auto *data = chunk.addBlockData(x, y, z, 9, 3);
			fillLoot(data->inventory, cell.lootSeed);
			chunk.setModified(true);
		}
		if (cell.block == spawner) {
			auto *data = chunk.addBlockData(x, y, z);
			data->meta.setInt("spawner_kind", 3);
			data->meta.setInt("spawner_delay", 20);
			data->meta.setInt("spawner_random", cell.lootSeed);
			chunk.setModified(true);
		}
	}
}
