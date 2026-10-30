#ifndef LUSHCAVESTESTS_HPP_
#define LUSHCAVESTESTS_HPP_

#include <array>
#include <vector>
#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/LushCaves.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class LushCavesTests : public CxxTest::TestSuite {
	static u16 id(const char *name) { return Registry::getInstance().getBlockFromStringID(name).id(); }

	static void definitions(Registry &registry) {
		Registry::setInstance(registry);
		auto &air = registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0);
		air.isCollidable(false); air.isOpaque(false);
		registry.registerItem<Item>({}, BLOCK_AIR, "air");
		for (const char *name : {"stone", "dirt", "moss_block", "rooted_dirt", "moss_carpet", "hanging_roots",
			"cave_vines", "azalea", "flowering_azalea", "spore_blossom", "copper_ore", "lava", "bedrock", "bricks"}) {
			auto &block = registry.registerBlock<ServerBlock>(std::string("default:") + name);
			std::string value(name);
			if (value == "moss_carpet" || value == "hanging_roots" || value == "cave_vines"
				|| value == "azalea" || value == "flowering_azalea" || value == "spore_blossom") {
				block.getState(0).isCollidable(false); block.getState(0).isOpaque(false);
			}
			if (value == "cave_vines") {
				block.addGroup("group:attached_node", 4); block.addGroup("group:om_hanging_chain", 1);
				block.addGroup("group:om_light_level", 5);
				block.getState(0).itemDrop("default:cave_vines");
				auto &berries = block.addState(); berries.isLightSource(true); berries.itemDrop("default:glow_berries");
				block.param().allocateBits(BlockParam::State, 1);
			}
		}
		registry.registerItem<Item>({}, "default:cave_vines", "vine");
		registry.registerItem<Item>({}, "default:glow_berries", "berries");
		registry.registerEntity("default:item_drop");
	}

	static void prepare(HeightmapChunk &base) {
		base.blocks().fill(0);
		for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) for (int z = 0; z <= 64; ++z) {
			int material = z == 0 ? 7 : z == 10 ? 11 : 1;
			if (z >= 16 && z <= 52 && (z % 8 == 0 || z % 8 >= 6)) material = 0;
			base.blocks()[(x * 16 + y) * 128 + z] = material;
		}
		base.refreshHeight();
	}

	static void fill(ServerChunk &chunk, const HeightmapChunk &base) {
		for (int z = 0; z < 32; ++z) for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
			int worldZ = z + chunk.z() * 32, material = base.blockAt(x, y, worldZ);
			u16 block = material == 1 ? id("default:stone") : material == 7 ? id("default:bedrock") : material == 11 ? id("default:lava") : 0;
			if ((x + y + worldZ) % 37 == 0) block = id(material ? "default:copper_ore" : "default:bricks");
			chunk.setBlockRaw(x, y, z, block);
		}
	}

	static std::uint64_t hash(const ServerChunk &chunk) {
		std::uint64_t result = 14695981039346656037ULL;
		for (int z = 0; z < 32; ++z) for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x)
			result = (result ^ chunk.getFullBlock(x, y, z)) * 1099511628211ULL;
		return result;
	}

public:
	void testLushPatchesKeepCaveGeometryMaterialsAndLoadOrder() {
		Registry registry; definitions(registry);
		PlayerList players; Dimension dimension(0, "test:overworld", "overworld");
		ServerWorld forward(players, dimension, 42), backward(players, dimension, 42);
		std::vector<Vector3i> positions;
		std::vector<std::uint64_t> expected;
		std::array<unsigned, 8> found{};
		const u16 additions[] = {id("default:moss_block"), id("default:rooted_dirt"), id("default:moss_carpet"), id("default:hanging_roots"),
			id("default:cave_vines"), id("default:azalea"), id("default:flowering_azalea"), id("default:spore_blossom")};
		unsigned berries = 0, borderVines = 0;
		for (int cy = -3; cy < 3; ++cy) for (int cx = -3; cx < 3; ++cx) {
			auto &base = forward.heightmap().getOrCreateChunk(cx, cy); prepare(base);
			auto immutable = base.blocks();
			for (int cz = 0; cz < 2; ++cz) {
				auto &chunk = forward.getOrCreateChunk(cx, cy, cz); fill(chunk, base);
				decorateLushCaves(chunk, forward.heightmap(), 42);
				TS_ASSERT(base.blocks() == immutable);
				for (int z = 0; z < 32; ++z) for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
					int worldZ = cz * 32 + z; u16 block = chunk.getBlock(x, y, z);
					if ((x + y + worldZ) % 37 == 0)
						TS_ASSERT_EQUALS(block, id(base.blockAt(x, y, worldZ) ? "default:copper_ore" : "default:bricks"));
					if (worldZ == 0 && (x + y) % 37 != 0) TS_ASSERT_EQUALS(block, id("default:bedrock"));
					if (worldZ == 10 && (x + y + 10) % 37 != 0) TS_ASSERT_EQUALS(block, id("default:lava"));
					for (unsigned i = 0; i < found.size(); ++i) if (block == additions[i]) {
						++found[i]; TS_ASSERT(worldZ >= 16 && worldZ <= 52);
						TS_ASSERT_EQUALS(base.blockAt(x, y, worldZ), i < 2 ? 1 : 0);
						if (i >= 2) TS_ASSERT(!registry.getBlock(block).getState(0).isCollidable());
					}
					if (block == id("default:cave_vines")) {
						if (z == 0 || z == 31) ++borderVines;
						if (chunk.getBlockState(x, y, z)->id() == 1) {
							++berries; TS_ASSERT_EQUALS(chunk.lightmap().getTorchlight(x, y, z), 5);
						}
					}
				}
				positions.push_back({cx, cy, cz}); expected.push_back(hash(chunk));
			}
		}
		for (unsigned count : found) TS_ASSERT(count > 0);
		TS_ASSERT(berries > 0); TS_ASSERT(borderVines > 0);
		for (std::size_t index = positions.size(); index-- > 0;) {
			const auto &p = positions[index];
			auto &base = backward.heightmap().getOrCreateChunk(p.x, p.y); prepare(base);
			auto &chunk = backward.getOrCreateChunk(p.x, p.y, p.z); fill(chunk, base);
			decorateLushCaves(chunk, backward.heightmap(), 42);
			TS_ASSERT_EQUALS(hash(chunk), expected[index]);
		}
	}

	void testHangingVineChainDropsItsNormalStateLootAfterCeilingRemoval() {
		Registry registry; definitions(registry);
		PlayerList players; Dimension dimension(0, "test:overworld", "overworld");
		ServerWorld world(players, dimension, 42);
		auto &chunk = world.getOrCreateChunk(0, 0, 0); chunk.setInitialized(true);
		world.setBlock(6, 6, 12, id("default:stone"));
		for (int z = 9; z < 12; ++z) world.setBlock(6, 6, z, id("default:cave_vines"));
		world.setBlockState(6, 6, 9, 1);
		for (int tick = 0; tick < 5; ++tick) world.blockPhysics().tick(8192);
		for (int z = 9; z < 12; ++z) TS_ASSERT_EQUALS(world.getBlock(6, 6, z), id("default:cave_vines"));
		world.setBlock(6, 6, 12, 0);
		for (int tick = 0; tick < 8; ++tick) world.blockPhysics().tick(8192);
		for (int z = 9; z < 12; ++z) TS_ASSERT_EQUALS(world.getBlock(6, 6, z), 0);
		unsigned vines = 0, berries = 0;
		world.scene().registry().view<ItemStack>().each([&](auto, auto &stack) {
			if (stack.stringID() == "default:cave_vines") vines += stack.amount();
			else if (stack.stringID() == "default:glow_berries") berries += stack.amount();
			else TS_FAIL("unexpected hanging vine drop");
		});
		TS_ASSERT_EQUALS(vines, 2); TS_ASSERT_EQUALS(berries, 1);
	}
};

#endif // LUSHCAVESTESTS_HPP_
