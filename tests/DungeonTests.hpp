#ifndef DUNGEONTESTS_HPP_
#define DUNGEONTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/core/GameClock.hpp"
#include "common/core/GameTime.hpp"
#include "common/core/LoggerHandler.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/world/Dimension.hpp"
#include "common/world/RetroDungeon.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/DungeonTerrain.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class DungeonTests : public CxxTest::TestSuite {
	static void definitions(Registry &registry) {
		Registry::setInstance(registry);
		auto &air = registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0); air.isOpaque(false); air.isCollidable(false);
		registry.registerItem<Item>({}, BLOCK_AIR, "air");
		for (const char *id : {"default:stone", "default:cobblestone", "default:cobblestone_mossy", "default:chest", "default:mob_spawner"})
			registry.registerBlock<ServerBlock>(id);
	}
	public:
		void setUp() override {
			static LoggerHandler logger; LoggerHandler::setInstance(logger);
			static GameClock clock; GameClock::setInstance(clock);
		}
		void testSourceRoomConstraintsMossFloorAndChestLoot() {
			unsigned floors = 0, moss = 0, chests = 0;
			for (int seed = 0; seed < 32; ++seed) {
				retro::Random shape(seed); int rx = shape.nextInt(2) + 2, ry = shape.nextInt(2) + 2;
				auto terrain = [=](int x, int y, int z) -> u8 { return x == rx + 1 && y == 0 && (z == 30 || z == 31) ? 0 : 1; };
				retro::Random random(seed); auto room = retro::makeDungeon(random, {0, 0, 30}, terrain);
				TS_ASSERT(room); if (!room) continue;
				TS_ASSERT_EQUALS(room->radiusX, rx); TS_ASSERT_EQUALS(room->radiusY, ry); TS_ASSERT(room->chests.size() <= 2);
				for (const auto &cell : room->cells) {
					TS_ASSERT(cell.position.z >= 29 && cell.position.z <= 33);
					if (cell.position.z == 29) { ++floors; if (cell.block == 48) ++moss; }
				}
				chests += room->chests.size();
				for (const auto &chest : room->chests) {
					unsigned stacks = 0;
					for (const auto &loot : chest.slots) if (loot.count) { ++stacks; TS_ASSERT(loot.count >= 1 && loot.count <= 4); }
					TS_ASSERT(stacks <= 9);
				}
				retro::Random closed(seed); TS_ASSERT(!retro::makeDungeon(closed, {0, 0, 30}, [](int, int, int) -> u8 { return 1; }));
				retro::Random roof(seed); TS_ASSERT(!retro::makeDungeon(roof, {0, 0, 30}, [&](int x, int y, int z) -> u8 { return z == 34 ? 0 : terrain(x, y, z); }));
				retro::Random flooded(seed); TS_ASSERT(!retro::makeDungeon(flooded, {0, 0, 30}, [&](int x, int y, int z) -> u8 { return z == 29 ? 8 : terrain(x, y, z); }));
				retro::Random openings(seed); TS_ASSERT(!retro::makeDungeon(openings, {0, 0, 30}, [=](int x, int y, int z) -> u8 {
					return (z == 30 || z == 31) && ((x == rx + 1 && std::abs(y) <= 2) || (x == -rx - 1 && y == 0)) ? 0 : 1;
				}));
			}
			TS_ASSERT(chests > 0); TS_ASSERT(moss > floors * 0.65 && moss < floors * 0.85);
		}

		void testRareLootHasOneWinningValueAndOrdinaryRollsExcludeIt() {
			for (int choice = 0; choice < 4; ++choice) {
				TS_ASSERT(std::string(retro::dungeonRareLoot(0, choice)).size() > 0);
				TS_ASSERT_EQUALS(std::string(retro::dungeonRareLoot(1, choice)), "");
				TS_ASSERT_EQUALS(std::string(retro::dungeonRareLoot(999999, choice)), "");
			}
			retro::Random random(42);
			for (int i = 0; i < 10000; ++i) {
				auto loot = retro::dungeonLoot(random);
				TS_ASSERT_DIFFERS(std::string(loot.id), "default:apple"); TS_ASSERT_DIFFERS(std::string(loot.id), "default:apple_golden");
				TS_ASSERT_DIFFERS(std::string(loot.id), "default:record_13"); TS_ASSERT_DIFFERS(std::string(loot.id), "default:record_cat");
			}
		}

		void testDungeonBordersDoNotDependOnGenerationOrderOrRewriteLoadedChunks() {
			Registry registry; definitions(registry); PlayerList players; Dimension dimension{0, "test:world", "world"};
			ServerWorld forward(players, dimension, 42), backward(players, dimension, 42);
			retro::Random random(42); int x = random.nextInt(16) + 8, z = random.nextInt(128), y = random.nextInt(16) + 8;
			int rx = random.nextInt(2) + 2;
			for (auto *world : {&forward, &backward}) {
				for (int cx = -2; cx <= 3; ++cx) for (int cy = -2; cy <= 3; ++cy) world->heightmap().getOrCreateChunk(cx, cy).blocks().fill(1);
				int openingX = x + rx + 1;
				auto &base = world->heightmap().getOrCreateChunk(openingX / 16, y / 16).blocks();
				base[((openingX & 15) * 16 + (y & 15)) * 128 + z] = 0;
				base[((openingX & 15) * 16 + (y & 15)) * 128 + z + 1] = 0;
			}
			std::vector<Vector3i> positions;
			for (int cx = 0; cx <= 1; ++cx) for (int cy = 0; cy <= 1; ++cy) for (int cz = 0; cz < 4; ++cz) positions.push_back({cx, cy, cz});
			auto generate = [&](ServerWorld &world, Vector3i position) {
				auto &chunk = world.getOrCreateChunk(position.x, position.y, position.z);
				auto &base = world.heightmap().getOrCreateChunk(position.x, position.y);
				for (int pz = 0; pz < 32; ++pz) for (int py = 0; py < 16; ++py) for (int px = 0; px < 16; ++px)
					chunk.setDataRaw(px, py, pz, base.blockAt(px, py, position.z * 32 + pz) ? 1 : 0);
				decorateDungeonTerrain(chunk, world.heightmap(), 42); chunk.setInitialized(true);
			};
			for (auto position : positions) generate(forward, position);
			std::reverse(positions.begin(), positions.end()); for (auto position : positions) generate(backward, position);
			unsigned spawners = 0;
			u16 spawner = registry.getBlockFromStringID("default:mob_spawner").id();
			for (auto position : positions) {
				auto *a = forward.getChunk(position.x, position.y, position.z), *b = backward.getChunk(position.x, position.y, position.z);
				for (int pz = 0; pz < 32; ++pz) for (int py = 0; py < 16; ++py) for (int px = 0; px < 16; ++px) {
					TS_ASSERT_EQUALS(a->data(px, py, pz), b->data(px, py, pz));
					if (a->getBlock(px, py, pz) == spawner) ++spawners;
				}
			}
			TS_ASSERT(spawners > 0);
			auto &edited = forward.getOrCreateChunk(x / 16, y / 16, z / 32); edited.setBlockRaw(x & 15, y & 15, z & 31, 1);
			decorateDungeonTerrain(edited, forward.heightmap(), 42); TS_ASSERT_EQUALS(edited.getBlock(x & 15, y & 15, z & 31), 1);
		}

		void testSpawnerRangeLightFourAttemptsAndSixMobCap() {
			Registry registry; definitions(registry);
			auto model = registry.registerEntity("default:zombie"); auto &models = registry.entityRegistry();
			models.emplace<MobComponent>(model).health = 20;
			models.emplace<BetaMobComponent>(model).kind = BetaMobKind::Zombie;
			models.emplace<DoubleBox>(model, -0.3, -0.3, 0, 0.6, 0.6, 1.8); models.emplace<RotationComponent>(model);
			PlayerList players; Dimension dimension{0, "test:world", "world"}; ServerWorld world(players, dimension, 42);
			auto &chunk = world.getOrCreateChunk(0, 0, 1); chunk.setInitialized(true);
			chunk.setBlockRaw(8, 8, 4, registry.getBlockFromStringID("default:mob_spawner").id());
			auto *data = world.addBlockData(8, 8, 36); data->meta.setInt("spawner_delay", 10); data->meta.setInt("spawner_kind", 1);
			world.tickSpawner(8, 8, 36); TS_ASSERT_EQUALS(data->meta.get<int>("spawner_delay"), 10);
			ClientInfo client{1, Network::LocalSession::connect()}; auto *player = players.connectPlayer("tester", client);
			TS_ASSERT(player); if (!player) return; player->setReady(true); player->setPosition(8, 15, 36);
			world.tickSpawner(8, 8, 36); TS_ASSERT_EQUALS(data->meta.get<int>("spawner_delay"), 9);
			GameTime::setTicks(6000);
			for (int pz = 0; pz < 32; ++pz) for (int py = 0; py < 16; ++py) for (int px = 0; px < 16; ++px) chunk.lightmap().setLightDataRaw(px, py, pz, 0xf0);
			data->meta.setInt("spawner_delay", 0); world.tickSpawner(8, 8, 36); TS_ASSERT_EQUALS(world.scene().registry().view<MobComponent>().size(), 0u);
			for (int pz = 0; pz < 32; ++pz) for (int py = 0; py < 16; ++py) for (int px = 0; px < 16; ++px) chunk.lightmap().setLightDataRaw(px, py, pz, 0);
			world.tickSpawner(8, 8, 36); TS_ASSERT(world.scene().registry().view<MobComponent>().size() <= 4u);
			for (int attempt = 0; attempt < 20; ++attempt) { data->meta.setInt("spawner_delay", 0); world.tickSpawner(8, 8, 36); }
			TS_ASSERT_EQUALS(world.scene().registry().view<MobComponent>().size(), 6u);
			TS_ASSERT(data->meta.get<int>("spawner_delay") >= 200 && data->meta.get<int>("spawner_delay") <= 799);
			Network::Packet packet; packet << data->meta; BlockMetadata restored; packet >> restored;
			TS_ASSERT_EQUALS(restored.get<int>("spawner_delay"), data->meta.get<int>("spawner_delay"));
			TS_ASSERT_EQUALS(restored.get<int>("spawner_random"), data->meta.get<int>("spawner_random"));
		}
};

#endif
