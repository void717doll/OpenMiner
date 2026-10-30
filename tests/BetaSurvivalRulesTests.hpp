#ifndef BETASURVIVALRULESTESTS_HPP_
#define BETASURVIVALRULESTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/inventory/Inventory.hpp"
#include "common/world/BetaPlayerMovement.hpp"
#include "common/world/BetaToolRules.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerItem.hpp"

class BetaSurvivalRulesTests : public CxxTest::TestSuite {
	static void definitions(Registry &registry) {
		Registry::setInstance(registry);
		registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
		registry.registerBlock<ServerBlock>("test:stone");
		registry.registerItem<Item>({}, BLOCK_AIR, "air");
		auto &tool = registry.registerItem<ServerItem>({}, "test:tool", "tool");
		tool.setMaxStackSize(1);
		tool.addGroup("group:om_max_damage", 59);
		registry.registerItem<ServerItem>({}, "test:stack", "stack");
	}

	public:
		void testLedgeValidationFollowsIndividualPhysicsSteps() {
			Registry registry; definitions(registry);
			PlayerList players;
			Dimension dimension{0, "test:world", "world"};
			ServerWorld world(players, dimension, 42);
			auto &chunk = world.getOrCreateChunk(0, 0, 0); chunk.setInitialized(true);
			for (int x = 0; x < 9; ++x) for (int y = 0; y < 16; ++y) chunk.setBlockRaw(x, y, 0, 1);
			auto &player = players.addPlayer("tester", false);
			ClientInfo client{1, {}}; player.setClient(&client); player.setReady(true);
			Vector3d start{9.15, 8.5, 1}; player.setPosition(start.x, start.y, start.z);
			DoubleBox box = player.hitbox(); box.move(start);
			Vector3d first = retro::movePlayer(world, box, {0.2, 0, -0.08}, true, false); box.move(first);
			Vector3d second = retro::movePlayer(world, box, {0.2, 0, -0.08}, true, false);
			Vector3d middle = start + first, end = middle + second;
			TS_ASSERT_EQUALS(first.z, 0);
			TS_ASSERT(second.z < 0);
			TS_ASSERT(!world.moveSurvivalPlayer(player, end.x, end.y, end.z));
			player.setPosition(start.x, start.y, start.z);
			TS_ASSERT(world.moveSurvivalPlayer(player, middle.x, middle.y, middle.z));
			TS_ASSERT(world.moveSurvivalPlayer(player, end.x, end.y, end.z));
			player.setHealth(0);
			TS_ASSERT(!world.moveSurvivalPlayer(player, end.x + 0.1, end.y, end.z));
		}

		void testMomentumFrictionJumpSwimmingAndSneaking() {
			retro::MovementEnvironment air;
			Vector3d walk{0, 0, 0}, sneak{0, 0, 0}, diagonal{0, 0, 0};
			for (int tick = 0; tick < 100; ++tick) {
				retro::acceleratePlayer(walk, 1, 0, 0, false, false, true, air);
				retro::dragPlayer(walk, true, false, air);
				retro::acceleratePlayer(sneak, 1, 0, 0, false, true, true, air);
				retro::dragPlayer(sneak, true, false, air);
				retro::acceleratePlayer(diagonal, 1, 1, 0, false, false, true, air);
				retro::dragPlayer(diagonal, true, false, air);
			}
			TS_ASSERT_DELTA(sneak.x / walk.x, 0.3, 1e-7);
			TS_ASSERT_DELTA(diagonal.x, diagonal.y, 1e-12);
			TS_ASSERT(walk.x > 0.117 && walk.x < 0.119);
			Vector3d jump{0, 0, 0};
			retro::acceleratePlayer(jump, 0, 0, 0, true, false, true, air);
			TS_ASSERT_EQUALS(jump.z, double(0.42f));
			double height = 0, peak = 0;
			for (int tick = 0; tick < 20; ++tick) {
				height += jump.z;
				peak = std::max(peak, height);
				retro::dragPlayer(jump, false, false, air);
			}
			TS_ASSERT_DELTA(peak, 1.252203, 1e-6);
			retro::MovementEnvironment water; water.water = true;
			Vector3d swimming{0, 0, 0};
			retro::acceleratePlayer(swimming, 1, 0, 0, true, false, false, water);
			TS_ASSERT_EQUALS(swimming.z, double(0.04f));
			retro::dragPlayer(swimming, false, false, water);
			TS_ASSERT_DELTA(swimming.z, 0.012, 1e-8);
			retro::MovementEnvironment lava; lava.lava = true;
			Vector3d falling{0.2, 0, -0.1};
			retro::dragPlayer(falling, false, false, lava);
			TS_ASSERT_DELTA(falling.x, 0.1, 1e-12);
			TS_ASSERT_DELTA(falling.z, -0.07, 1e-12);
		}

		void testAxisOrderSlidesAlongWallsAndStopsAtFloor() {
			std::vector<collision::Obstacle> boxes{{DoubleBox{-5, -5, -1, 10, 10, 1}}, {DoubleBox{1, -5, 0, 1, 10, 5}}};
			DoubleBox body{0, 0, 0, 0.6, 0.6, 1.8};
			Vector3d move = retro::movePlayerBox(boxes, body, {1, 0.5, -0.2});
			TS_ASSERT_DELTA(move.x, 0.4, 1e-12);
			TS_ASSERT_EQUALS(move.y, 0.5);
			TS_ASSERT_EQUALS(move.z, 0);
			retro::MovementEnvironment ladder; ladder.ladder = true;
			Vector3d velocity{1, -1, -1};
			retro::acceleratePlayer(velocity, 0, 0, 0, false, true, false, ladder);
			TS_ASSERT_EQUALS(velocity.x, double(0.15f));
			TS_ASSERT_EQUALS(velocity.z, 0);
			retro::dragPlayer(velocity, false, true, ladder);
			TS_ASSERT_DELTA(velocity.z, (0.2 - 0.08) * double(0.98f), 1e-12);
		}

		void testHalfBlockStepSneakEdgeAndServerAcceptance() {
			Registry registry; definitions(registry);
			auto &slab = registry.registerBlock<ServerBlock>("test:slab");
			slab.getState(0).boundingBox({0, 0, 0, 1, 1, 0.5});
			PlayerList players;
			Dimension dimension{0, "test:world", "test"};
			ServerWorld world(players, dimension, 42);
			auto &chunk = world.getOrCreateChunk(0, 0, 0);
			chunk.setInitialized(true);
			for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) chunk.setBlockRaw(x, y, 0, 1);
			chunk.setBlockRaw(9, 8, 1, slab.id());
			DoubleBox body{8.3, 8.2, 1, 0.6, 0.6, 1.8};
			auto step = retro::movePlayer(world, body, {0.3, 0, -0.08}, true, false);
			TS_ASSERT_DELTA(step.x, 0.3, 1e-12);
			TS_ASSERT_DELTA(step.z, 0.5, 1e-12);
			auto &player = players.addPlayer("tester", true);
			ClientInfo client{1, {}}; player.setClient(&client); player.setReady(true);
			player.setPosition(8.6, 8.5, 1);
			TS_ASSERT(world.moveSurvivalPlayer(player, 8.9, 8.5, 1.5));
			for (int x = 9; x < 16; ++x) for (int y = 0; y < 16; ++y) {
				chunk.setBlockRaw(x, y, 0, 0); chunk.setBlockRaw(x, y, 1, 0);
			}
			body.reset(8.65, 8.2, 1, 0.6, 0.6, 1.8);
			auto sneaking = retro::movePlayer(world, body, {0.4, 0, -0.08}, true, true);
			TS_ASSERT(sneaking.x < 0.36);
			TS_ASSERT(sneaking.x > 0.25);
		}

		void testDurabilityBoundaryStackConservationAndPacketBoundary() {
			Registry registry; definitions(registry);
			ItemStack tool{"test:tool", 1};
			TS_ASSERT(tool.wear(59));
			TS_ASSERT_EQUALS(tool.amount(), 1);
			TS_ASSERT_EQUALS(tool.damage(), 59);
			sf::Packet packet;
			packet << tool << ItemStack{"test:stack", 17} << u32(0x12345678);
			ItemStack restored, next; u32 marker = 0;
			packet >> restored >> next >> marker;
			TS_ASSERT(packet && packet.endOfPacket());
			TS_ASSERT_EQUALS(restored.damage(), 59);
			TS_ASSERT_EQUALS(next.amount(), 17);
			TS_ASSERT_EQUALS(marker, 0x12345678u);
			TS_ASSERT(restored.wear(1));
			TS_ASSERT_EQUALS(restored.amount(), 0);
			Inventory inventory{2, 1};
			TS_ASSERT_EQUALS(inventory.addItemStack(tool).amount(), 0);
			TS_ASSERT_EQUALS(inventory.addItemStack(ItemStack{"test:tool", 1, 12}).amount(), 0);
			TS_ASSERT_EQUALS(inventory.addItemStack(tool).amount(), 1);
			TS_ASSERT_EQUALS(inventory.getStack(0, 0).damage(), 59);
			TS_ASSERT_EQUALS(inventory.getStack(1, 0).damage(), 12);
			sf::Packet saved; saved << inventory << u32(0x12345678);
			Inventory loaded; saved >> loaded >> marker;
			TS_ASSERT(saved && saved.endOfPacket());
			TS_ASSERT_EQUALS(marker, 0x12345678u);
			TS_ASSERT_EQUALS(loaded.getStack(0, 0).damage(), 59);
			TS_ASSERT_EQUALS(loaded.getStack(1, 0).damage(), 12);
			ItemStack legacy{"test:tool", 3, 59}; legacy.wear(1);
			TS_ASSERT_EQUALS(legacy.amount(), 2);
			TS_ASSERT_EQUALS(legacy.damage(), 0);
			Inventory oversized{1, 1}; oversized.setStack(0, 0, "test:stack", 100);
			TS_ASSERT_EQUALS(oversized.addStack("test:stack", 5).amount(), 5);
			TS_ASSERT_EQUALS(oversized.getStack(0, 0).amount(), 100);
		}

		void testDeathDropsKeepWearAndMobLootStaysOnGround() {
			Registry registry; definitions(registry);
			auto drop = registry.registerEntity("default:item_drop");
			registry.entityRegistry().emplace<DoubleBox>(drop, -0.125, -0.125, 0, 0.25, 0.25, 0.25);
			auto model = registry.registerEntity("test:pig");
			auto &mob = registry.entityRegistry().emplace<MobComponent>(model);
			mob.health = 1; mob.dropItem = "test:stack";
			registry.entityRegistry().emplace<DoubleBox>(model, -0.3, -0.3, 0, 0.6, 0.6, 1.8);
			registry.entityRegistry().emplace<RotationComponent>(model);
			registry.entityRegistry().emplace<BetaMobComponent>(model).kind = BetaMobKind::Pig;
			PlayerList players;
			Dimension dimension{0, "test:world", "test"};
			ServerWorld world(players, dimension, 42);
			auto &chunk = world.getOrCreateChunk(0, 0, 0); chunk.setInitialized(true);
			for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) chunk.setBlockRaw(x, y, 0, 1);
			auto &player = players.addPlayer("tester", true);
			player.setPosition(8, 8, 1); player.setHeldItemSlot(0);
			player.inventory().setItemStack(0, 0, {"test:tool", 1, 42});
			world.dropPlayerItems(player);
			TS_ASSERT_EQUALS(player.inventory().getStack(0, 0).amount(), 0);
			unsigned worn = 0;
			world.scene().registry().view<ItemStack>().each([&](auto &stack) {
				if (stack.item().stringID() == "test:tool") { ++worn; TS_ASSERT_EQUALS(stack.damage(), 42); }
			});
			TS_ASSERT_EQUALS(worn, 1u);
			player.setRotation(0, 0);
			TS_ASSERT(world.spawnMob("test:pig", 10, 8, 1));
			auto target = world.findMobTarget(player, 3);
			TS_ASSERT(target != entt::null);
			world.scene().registry().get<MobComponent>(target).randomState = 123;
			TS_ASSERT(world.attackMob(player));
			world.updateMobs();
			unsigned loot = 0;
			world.scene().registry().view<ItemStack>().each([&](auto &stack) {
				if (stack.item().stringID() == "test:stack") loot += stack.amount();
			});
			TS_ASSERT(loot > 0);
			TS_ASSERT_EQUALS(player.inventory().getStack(0, 0).amount(), 0);
		}

		void testMiningPenaltiesAndWearDifferByAction() {
			Block dirt{1, "default:dirt"}; dirt.getState(0).hardness(0.5);
			Item sword{1, {}, "default:wooden_sword", "sword"};
			sword.addGroup("group:om_tool_type", 1);
			TS_ASSERT_DELTA(retro::survivalBreakTime(dirt.getState(0), sword, false, true), 0.5, 1e-7);
			TS_ASSERT_DELTA(retro::survivalBreakTime(dirt.getState(0), sword, true, false), 12.5, 1e-7);
			TS_ASSERT_EQUALS(retro::diggingWear(sword, dirt.getState(0)), 2);
			TS_ASSERT_EQUALS(retro::attackingWear(sword), 1);
			Item pickaxe{2, {}, "default:wooden_pickaxe", "pickaxe"};
			pickaxe.addGroup("group:om_tool_type", 2);
			TS_ASSERT_EQUALS(retro::diggingWear(pickaxe, dirt.getState(0)), 1);
			TS_ASSERT_EQUALS(retro::attackingWear(pickaxe), 2);
		}
};

#endif // BETASURVIVALRULESTESTS_HPP_
