#ifndef PLAYERSURVIVALTESTS_HPP_
#define PLAYERSURVIVALTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include <limits>
#include "common/core/Registry.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/PlayerSurvival.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class PlayerSurvivalTests : public CxxTest::TestSuite {
	static void registerBlocks(Registry &definitions) {
		Registry::setInstance(definitions);
		definitions.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
		definitions.registerBlock<ServerBlock>("default:stone");
		for (const char *id : {"default:water", "default:lava"}) {
			auto &state = definitions.registerBlock<ServerBlock>(id).getState(0);
			state.isCollidable(false);
			state.isOpaque(false);
			state.drawType(BlockDrawType::Liquid);
		}
		auto &cactus = definitions.registerBlock<ServerBlock>("default:cactus").getState(0);
		cactus.isOpaque(false);
		cactus.boundingBox({1.0 / 16, 1.0 / 16, 0, 14.0 / 16, 14.0 / 16, 1});
	}

	static void prepareFloor(ServerWorld &world) {
		auto &chunk = world.getOrCreateChunk(0, 0, 0);
		for (int x = 0; x < CHUNK_WIDTH; ++x)
			for (int y = 0; y < CHUNK_DEPTH; ++y) chunk.setBlockRaw(x, y, 0, 1);
		chunk.setInitialized(true);
	}

	public:
		void testDamageCauseMatchesTheStrongestActiveHazard() {
			PlayerSurvivalState state;
			PlayerEnvironment environment;
			environment.lava = true; environment.cactus = true;
			TS_ASSERT_EQUALS(state.tick(environment), 4);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "lava");
			state = {}; environment = {}; environment.submerged = true;
			state.air = -19;
			TS_ASSERT_EQUALS(state.tick(environment), 2);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "drowning");
			state = {}; environment = {}; environment.cactus = true;
			TS_ASSERT_EQUALS(state.tick(environment), 1);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "cactus");
			state = {}; environment = {}; environment.suffocating = true;
			TS_ASSERT_EQUALS(state.tick(environment), 1);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "suffocation");
			state = {}; state.fireTicks = 20; environment = {};
			TS_ASSERT_EQUALS(state.tick(environment), 1);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "fire");
			state = {}; state.fallDistance = 6;
			TS_ASSERT_EQUALS(state.moved(0, true, false), 3);
			TS_ASSERT_EQUALS(std::string(state.damageCause), "fall");
		}

		void testBetaFallThresholdAndWaterReset() {
			PlayerSurvivalState state;
			TS_ASSERT_EQUALS(state.moved(-2, false, false), 0);
			TS_ASSERT_EQUALS(state.moved(-1, true, false), 0);
			TS_ASSERT_EQUALS(state.fallDistance, 0);
			TS_ASSERT_EQUALS(state.moved(-3.01, true, false), 1);
			TS_ASSERT_EQUALS(state.moved(-5.1, true, false), 3);
			TS_ASSERT_EQUALS(state.moved(1, false, false), 0);
			TS_ASSERT_EQUALS(state.fallDistance, 0);
			TS_ASSERT_EQUALS(state.moved(-8, false, false), 0);
			TS_ASSERT_EQUALS(state.moved(-1, true, true), 0);
			TS_ASSERT_EQUALS(state.fallDistance, 0);
		}

		void testAirLasts300TicksThenDrowningEvery20Ticks() {
			PlayerSurvivalState state;
			PlayerEnvironment water;
			water.water = water.submerged = true;
			for (int tick = 0; tick < 319; ++tick) TS_ASSERT_EQUALS(state.tick(water), 0);
			TS_ASSERT_EQUALS(state.air, -19);
			TS_ASSERT_EQUALS(state.tick(water), 2);
			TS_ASSERT_EQUALS(state.air, 0);
			for (int tick = 0; tick < 19; ++tick) TS_ASSERT_EQUALS(state.tick(water), 0);
			TS_ASSERT_EQUALS(state.tick(water), 2);
			TS_ASSERT_EQUALS(state.tick({}), 0);
			TS_ASSERT_EQUALS(state.air, 300);
		}

		void testLavaBurnDurationExtinguishingAndDamageGrace() {
			PlayerSurvivalState state;
			PlayerEnvironment lava;
			lava.lava = true;
			unsigned received = 0;
			for (int tick = 0; tick < 31; ++tick) received += state.filterDamage(state.tick(lava));
			TS_ASSERT_EQUALS(received, 16);
			TS_ASSERT_EQUALS(state.fireTicks, 600);
			state = {};
			state.fireTicks = 600;
			unsigned burns = 0;
			for (int tick = 0; tick < 600; ++tick) burns += state.tick({});
			TS_ASSERT_EQUALS(burns, 30);
			state.fireTicks = 200;
			PlayerEnvironment water;
			water.water = true;
			TS_ASSERT_EQUALS(state.tick(water), 0);
			TS_ASSERT_EQUALS(state.fireTicks, -20);
			state = {};
			TS_ASSERT_EQUALS(state.filterDamage(2), 2);
			TS_ASSERT_EQUALS(state.filterDamage(1), 0);
			TS_ASSERT_EQUALS(state.filterDamage(4), 2);
			for (int tick = 0; tick < 10; ++tick) state.tick({});
			TS_ASSERT_EQUALS(state.filterDamage(2), 2);
		}

		void testLoadedMaterialContactAndDrowningDamage() {
			Registry definitions;
			registerBlocks(definitions);
			PlayerList players;
			auto &player = players.addPlayer("tester", true);
			ClientInfo client{1, {}};
			player.setClient(&client);
			player.setReady(true);
			player.setPosition(8.5, 8.5, 1);
			Dimension dimension{0, "test:world", "test"};
			ServerWorld world(players, dimension, 42);
			prepareFloor(world);
			auto *chunk = world.getChunkAtBlockPos(8, 8, 1);
			chunk->setBlockRaw(8, 8, 1, 2);
			chunk->setBlockRaw(8, 8, 2, 2);
			DoubleBox box = player.hitbox(); box.move(player.x(), player.y(), player.z());
			auto environment = playerEnvironment(world, box);
			TS_ASSERT(environment.loaded && environment.grounded && environment.water && environment.submerged);
			TS_ASSERT(!environment.lava && !environment.cactus && !environment.suffocating);
			for (int tick = 0; tick < 319; ++tick) world.updatePlayerSurvival();
			TS_ASSERT_EQUALS(player.health(), 20);
			world.updatePlayerSurvival();
			TS_ASSERT_EQUALS(player.health(), 18);
			chunk->setBlockRaw(8, 8, 1, 3);
			chunk->setBlockRaw(8, 8, 2, 0);
			environment = playerEnvironment(world, box);
			TS_ASSERT(environment.lava && !environment.water && !environment.submerged);
			player.resetSurvival();
			world.updatePlayerSurvival();
			TS_ASSERT_EQUALS(player.health(), 14);
			chunk->setBlockRaw(8, 8, 1, 0);
			chunk->setBlockRaw(8, 8, 2, 1);
			TS_ASSERT(playerEnvironment(world, box).suffocating);
			chunk->setBlockRaw(8, 8, 2, 0);
			chunk->setBlockRaw(9, 8, 1, 4);
			DoubleBox cactusContact = player.hitbox(); cactusContact.move(8.75, 8.5, 1);
			TS_ASSERT(playerEnvironment(world, cactusContact).cactus);
			player.setReady(false);
			for (int tick = 0; tick < 40; ++tick) world.updatePlayerSurvival();
			TS_ASSERT_EQUALS(player.health(), 14);
			player.setReady(true);
			chunk->setInitialized(false);
			TS_ASSERT(!playerEnvironment(world, box).loaded);
			for (int tick = 0; tick < 40; ++tick) world.updatePlayerSurvival();
			TS_ASSERT_EQUALS(player.health(), 14);
		}

		void testMovementCollisionAndLandingAreServerChecked() {
			Registry definitions;
			registerBlocks(definitions);
			PlayerList players;
			auto &player = players.addPlayer("tester", true);
			ClientInfo client{1, {}};
			player.setClient(&client);
			player.setPosition(8.5, 8.5, 6);
			Dimension dimension{0, "test:world", "test"};
			ServerWorld world(players, dimension, 42);
			prepareFloor(world);
			TS_ASSERT(!world.moveSurvivalPlayer(player, 8.5, 8.5, 5));
			player.setReady(true);
			for (int z = 5; z >= 1; --z) TS_ASSERT(world.moveSurvivalPlayer(player, 8.5, 8.5, z));
			TS_ASSERT_EQUALS(player.health(), 18);
			TS_ASSERT_EQUALS(player.survival().fallDistance, 0);
			auto *chunk = world.getChunkAtBlockPos(9, 8, 1);
			chunk->setBlockRaw(9, 8, 1, 1);
			chunk->setBlockRaw(9, 8, 2, 1);
			TS_ASSERT(!world.moveSurvivalPlayer(player, 10.5, 8.5, 1));
			TS_ASSERT_EQUALS(player.x(), 8.5);
			TS_ASSERT(!world.moveSurvivalPlayer(player, std::numeric_limits<double>::infinity(), 8.5, 1));
			TS_ASSERT(!world.moveSurvivalPlayer(player, 100, 8.5, 1));
			player.resetSurvival();
			TS_ASSERT_EQUALS(player.survival().air, 300);
			TS_ASSERT_EQUALS(player.survival().hurtTicks, 0);
		}
};

#endif // PLAYERSURVIVALTESTS_HPP_
