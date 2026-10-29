#ifndef RETROMOBTESTS_HPP_
#define RETROMOBTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/RetroTerrain.hpp"
#include "common/core/Registry.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerBlock.hpp"
#include "common/world/Player.hpp"
#include "common/scene/RetroMobMesh.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/MobComponent.hpp"

class RetroMobTests : public CxxTest::TestSuite {
	public:
		void testHealthAndFoodHealing() {
			Player player;
			player.damage(5);
			TS_ASSERT_EQUALS(player.health(), 15);
			player.heal(3);
			TS_ASSERT_EQUALS(player.health(), 18);
			player.heal(42);
			TS_ASSERT_EQUALS(player.health(), 20);
			player.damage(100);
			TS_ASSERT_EQUALS(player.health(), 0);
		}

		void testFoodStackLimits() {
			Registry definitions;
			Registry::setInstance(definitions);
			definitions.registerItem<Item>({}, BLOCK_AIR, "air");
			definitions.registerItem<Item>({}, "test:food", "food").setMaxStackSize(1);
			Inventory inventory(3, 1);
			TS_ASSERT_EQUALS(inventory.addStack("test:food", 2).amount(), 0);
			TS_ASSERT_EQUALS(inventory.getStack(0, 0).amount(), 1);
			TS_ASSERT_EQUALS(inventory.getStack(1, 0).amount(), 1);
		}

		void testMobModels() {
			for (unsigned model : {1u, 2u}) {
				auto vertices = retro::mobMesh(model);
				TS_ASSERT_EQUALS(vertices.size(), 216);
				for (const auto &vertex : vertices) {
					TS_ASSERT(std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z));
					TS_ASSERT(vertex.u >= 0 && vertex.u <= 1);
					TS_ASSERT(vertex.v >= 0 && vertex.v <= 1);
				}
				DrawableDef definition;
				definition.setMobModel(u8(model));
				sf::Packet packet;
				packet << definition;
				DrawableDef restored;
				packet >> restored;
				TS_ASSERT_EQUALS(restored.mobModel(), model);
			}
		}

		void testMobSpawnAndTerrainCollision() {
			Registry definitions;
			Registry::setInstance(definitions);
			definitions.registerBlock<ServerBlock>("air").getState(0).isCollidable(false);
			definitions.registerBlock<ServerBlock>("default:stone");
			auto model = definitions.registerEntity("test:pig");
			auto &models = definitions.entityRegistry();
			models.emplace<MobComponent>(model);
			models.emplace<DoubleBox>(model, -0.45, -0.45, 0, 0.9, 0.9, 0.9);
			models.emplace<RotationComponent>(model);
			PlayerList players;
			Dimension dimension{0, "test:world", "test"};
			ServerWorld world(players, dimension, 42);
			auto &chunk = world.getOrCreateChunk(0, 0, 0);
			for (int x = 0; x < 16; ++x)
				for (int y = 0; y < 16; ++y) chunk.setBlockRaw(x, y, 0, 1);
			chunk.setInitialized(true);
			TS_ASSERT(world.spawnMob("test:pig", 8, 8, 1));
			TS_ASSERT(!world.spawnMob("test:pig", 8, 8, 0));
			TS_ASSERT(!world.spawnMob("test:pig", 100, 100, 1));
			for (int tick = 0; tick < 40; ++tick) world.updateMobs();
			world.scene().registry().view<MobComponent, PositionComponent>().each([&](auto, auto &mob, auto &position) {
				TS_ASSERT_EQUALS(mob.health, 10);
				TS_ASSERT_DELTA(position.z, 1, 0.001);
				TS_ASSERT(std::abs(position.x - 8) + std::abs(position.y - 8) > 0.1);
			});
		}

		void testMobSaveState() {
			MobComponent mob;
			mob.hostile = true; mob.health = 7; mob.age = 99; mob.dropItem = "default:feather";
			sf::Packet packet;
			packet << mob;
			MobComponent restored;
			packet >> restored;
			TS_ASSERT_EQUALS(restored.health, 7);
			TS_ASSERT_EQUALS(restored.age, 99);
			TS_ASSERT_EQUALS(restored.dropItem, "default:feather");
			TS_ASSERT(restored.hostile);
		}
};

#endif // RETROMOBTESTS_HPP_
