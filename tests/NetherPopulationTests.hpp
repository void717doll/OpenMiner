#ifndef NETHERPOPULATIONTESTS_HPP_
#define NETHERPOPULATIONTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/Heightmap.hpp"
#include "common/core/Registry.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/network/Server.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class NetherPopulationTests : public CxxTest::TestSuite {
	static std::uint64_t hash(const retro::Terrain::Blocks &blocks) {
		std::uint64_t value = 14695981039346656037ULL;
		for (auto block : blocks) value = (value ^ block) * 1099511628211ULL;
		return value;
	}
public:
	void testGlowstoneMatchesBothOriginalJavaGenerators() {
		const std::uint64_t expected[] = {0xcbe5e00d129db60fULL, 0xbb583a56b5688132ULL, 0xdc1845833f302316ULL, 0xa4252691b13b82edULL};
		unsigned sample = 0;
		for (std::int64_t seed : {0LL, 42LL, -99LL, 9876543210123456LL}) {
			std::array<std::uint8_t, 32 * 32 * 128> blocks{};
			for (int x = 0; x < 32; ++x)
				for (int y = 0; y < 32; ++y) {
					blocks[(x * 32 + y) * 128] = 7;
					blocks[(x * 32 + y) * 128 + 90] = 87;
				}
			auto read = [&](int x, int y, int z) {
				if (z < 0) return 7;
				if (x < 0 || x >= 32 || y < 0 || y >= 32 || z >= 128) return 0;
				return int(blocks[(x * 32 + y) * 128 + z]);
			};
			auto write = [&](int x, int y, int z) { blocks[(x * 32 + y) * 128 + z] = 89; };
			retro::Random random(seed);
			TS_ASSERT(retro::glowstoneCluster(random, 16, 16, 89, read, write));
			std::uint64_t actual = 14695981039346656037ULL;
			for (auto block : blocks) actual = (actual ^ block) * 1099511628211ULL;
			TS_ASSERT_EQUALS(actual, expected[sample++]);
			TS_ASSERT(!retro::glowstoneCluster(random, 8, 8, 60, read, write));
			TS_ASSERT(!retro::glowstoneCluster(random, 16, 16, 89, read, write));
		}
	}

	void testGlowstoneBordersLoadOrderAndReseeding() {
		Heightmap forward(42, true), backward(42, true);
		std::array<std::uint64_t, 64> expected;
		unsigned glowstone = 0, crossed = 0;
		for (int y = -4; y < 4; ++y) {
			for (int x = -4; x < 4; ++x) {
				auto &chunk = forward.getOrCreateChunk(x, y);
				auto base = chunk.blocks();
				chunk.populateNether();
				expected[(y + 4) * 8 + x + 4] = hash(chunk.blocks());
				for (unsigned i = 0; i < base.size(); ++i) {
					if (chunk.blocks()[i] == 89) { TS_ASSERT_EQUALS(base[i], 0); ++glowstone; }
					else TS_ASSERT_EQUALS(chunk.blocks()[i], base[i]);
				}
				for (const auto &position : forward.netherGlowstone(x, y))
					if (position.x >= (x + 1) * 16 || position.y >= (y + 1) * 16) ++crossed;
			}
		}
		TS_ASSERT(glowstone > 0); TS_ASSERT(crossed > 0);
		for (int y = 3; y >= -4; --y)
			for (int x = 3; x >= -4; --x) {
				auto &chunk = backward.getOrCreateChunk(x, y); chunk.populateNether();
				TS_ASSERT_EQUALS(hash(chunk.blocks()), expected[(y + 4) * 8 + x + 4]);
			}
		forward.setSeed(43);
		auto &changed = forward.getOrCreateChunk(0, 0); changed.populateNether();
		TS_ASSERT_DIFFERS(hash(changed.blocks()), expected[36]);
		forward.setSeed(42);
		auto &restored = forward.getOrCreateChunk(0, 0); restored.populateNether();
		TS_ASSERT_EQUALS(hash(restored.blocks()), expected[36]);
		Heightmap overworld(42);
		auto &surface = overworld.getOrCreateChunk(0, 0); auto before = hash(surface.blocks());
		surface.populateNether(); TS_ASSERT_EQUALS(hash(surface.blocks()), before);
	}

	void testGeneratedGlowstoneIsMappedAndLit() {
		Registry definitions; Registry::setInstance(definitions);
		definitions.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
		for (const std::string name : {"netherrack", "bedrock", "lava", "gravel", "soul_sand", "glowstone"})
			{
				auto &block = definitions.registerBlock<ServerBlock>("default:" + name);
				if (name == "glowstone") {
					block.getState(0).isLightSource(true);
					block.addGroup("group:om_light_level", 14);
				}
			}
		u16 stone = definitions.getBlockFromStringID("default:netherrack").id();
		u16 glow = definitions.getBlockFromStringID("default:glowstone").id();
		auto &biome = definitions.registerBiome("test:nether", "nether");
		biome.addParameter(0); biome.addParameter(0);
		biome.setBeachBlockID(stone); biome.setPortalBlockID(0); biome.setPortalFrameBlockID(0);
		biome.setTopBlockID(stone); biome.setGroundBlockID(stone); biome.setDeepBlockID(stone);
		biome.setLiquidBlockID(definitions.getBlockFromStringID("default:lava").id());
		Dimension dimension(1, "test:nether", "nether"); dimension.addBiome("test:nether");
		PlayerList players; ServerWorld world(players, dimension, 42);
		Vector3i position; bool found = false;
		for (int x = -4; x < 4 && !found; ++x)
			for (int y = -4; y < 4 && !found; ++y) {
				const auto &points = world.heightmap().netherGlowstone(x, y);
				if (!points.empty()) { position = points.front(); found = true; }
			}
		TS_ASSERT(found); if (!found) return;
		int cx = int(std::floor(position.x / 16.0)), cy = int(std::floor(position.y / 16.0));
		auto &chunk = world.getOrCreateChunk(cx, cy, position.z / 32);
		world.generateChunk(chunk);
		int x = position.x - cx * 16, y = position.y - cy * 16, z = position.z % 32;
		TS_ASSERT_EQUALS(chunk.getBlock(x, y, z), glow);
		TS_ASSERT_EQUALS(chunk.lightmap().getTorchlight(x, y, z), 14);
	}

	void testNaturalNetherSpawningRejectsPlayersLiquidsAndOverlaps() {
		Registry definitions; Registry::setInstance(definitions);
		definitions.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
		definitions.registerBlock<ServerBlock>("test:stone");
		auto &lava = definitions.registerBlock<ServerBlock>("test:lava").getState(0);
		lava.isCollidable(false); lava.drawType(BlockDrawType::Liquid);
		for (bool ghast : {false, true}) {
			auto model = definitions.registerEntity(ghast ? "default:ghast" : "default:pig_zombie");
			auto &models = definitions.entityRegistry();
			models.emplace<MobComponent>(model).hostile = ghast;
			models.emplace<BetaMobComponent>(model).kind = ghast ? BetaMobKind::Ghast : BetaMobKind::PigZombie;
			double width = ghast ? 4 : 0.6;
			models.emplace<DoubleBox>(model, -width / 2, -width / 2, 0, width, width, ghast ? 4 : 1.8);
			models.emplace<RotationComponent>(model);
		}
		PlayerList players; Dimension dimension(1, "test:nether", "nether");
		ServerWorld world(players, dimension, 42);
		for (int cy = -3; cy <= 3; ++cy)
			for (int cx = -3; cx <= 3; ++cx) {
				auto &chunk = world.getOrCreateChunk(cx, cy, 1);
				for (int z = 0; z < 32; z += 8)
					for (int y = 0; y < 16; ++y)
						for (int x = 0; x < 16; ++x) chunk.setBlockRaw(x, y, z, cx == 3 ? 2 : 1);
				chunk.setInitialized(true);
			}
		ClientInfo client{1, Network::LocalSession::connect()};
		auto *player = players.connectPlayer("tester", client); TS_ASSERT(player); if (!player) return;
		player->setDimension(1); player->setPosition(0.5, 0.5, 49); player->setReady(true);
		unsigned ghasts = 0, pigmen = 0;
		auto &registry = world.scene().registry();
		for (int pass = 0; pass < 200; ++pass) {
			for (int tick = 0; tick < 20; ++tick) world.updateMobs();
			std::vector<DoubleBox> boxes;
			registry.view<MobComponent, BetaMobComponent, PositionComponent, DoubleBox>().each([&](auto, auto &, auto &traits, auto &position, auto &localBox) {
				if (traits.kind == BetaMobKind::Ghast) ++ghasts;
				else { TS_ASSERT(traits.kind == BetaMobKind::PigZombie); ++pigmen; }
				double dx = position.x - player->x(), dy = position.y - player->y(), dz = position.z - player->z();
				TS_ASSERT(dx * dx + dy * dy + dz * dz >= 24 * 24);
				TS_ASSERT_EQUALS(world.getBlock(int(std::floor(position.x)), int(std::floor(position.y)), int(position.z) - 1), 1);
				DoubleBox box = localBox; box.move(position.x, position.y, position.z);
				for (const auto &other : boxes) TS_ASSERT(!box.intersects(other));
				boxes.push_back(box);
			});
			TS_ASSERT(boxes.size() <= 64); registry.clear();
		}
		TS_ASSERT(ghasts > 0); TS_ASSERT(pigmen > ghasts);
		player->setReady(false);
		for (int tick = 0; tick < 100; ++tick) world.updateMobs();
		TS_ASSERT_EQUALS(registry.view<MobComponent>().size(), 0);
	}
};
#endif
