#ifndef WORLDSYSTEMSTESTS_HPP_
#define WORLDSYSTEMSTESTS_HPP_

#include <cxxtest/TestSuite.h>

#include "common/core/Registry.hpp"
#include "common/world/WorldCollision.hpp"
#include "common/world/Heightmap.hpp"
#include "common/world/Player.hpp"

class WorldSystemsTests : public CxxTest::TestSuite {
	public:
		class TestChunk : public Chunk {
			public:
				TestChunk(World &world) : Chunk(0, 0, 0, world) {}
				void update() override {}
				void process() override {}
		};
		class TestWorld : public World {
			public:
				TestWorld() : chunk(*this) {}
				Chunk *getChunk(int x, int y, int z) const override {
					return x == 0 && y == 0 && z == 0 ? const_cast<TestChunk *>(&chunk) : nullptr;
				}
				TestChunk chunk;
		};

		void testPlacementGroups() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air");
			Block &soil = registry.registerBlock<Block>("default:soil");
			soil.addGroup("group:soil", 1);
			TestWorld world;
			world.chunk.setBlockRaw(1, 1, 0, soil.id());
			BlockPlacementConstraints constraints;
			BlockPlacementConstraint constraint;
			constraint.blockOffset = {0, 0, -1};
			constraint.blockID = "group:soil";
			constraints.addConstraint(constraint);
			TS_ASSERT(constraints.check(world, {1, 1, 1}));
			TS_ASSERT(!constraints.check(world, {2, 2, 1}));
			TS_ASSERT(!constraints.check(world, {1, 1, 0}));
		}

		void testMobRaySelection() {
			DoubleBox box{2, -0.5, 0, 1, 1, 2};
			TS_ASSERT_EQUALS(collision::rayDistance({0, 0, 1}, {1, 0, 0}, box, 3), 2);
			TS_ASSERT_EQUALS(collision::rayDistance({0, 1, 1}, {1, 0, 0}, box, 3), -1);
			TS_ASSERT_EQUALS(collision::rayDistance({0, 0, 1}, {-1, 0, 0}, box, 3), -1);
			TS_ASSERT_EQUALS(collision::rayDistance({0, 0, 1}, {1, 0, 0}, box, 1), -1);
			TS_ASSERT_EQUALS(collision::rayDistance({2.5, 0, 1}, {0, 0, 1}, box, 3), 0);
		}

		void testSweptCollision() {
			DoubleBox player{0, 0, 0, 0.5, 0.5, 1.75};
			DoubleBox wall{2, 0, 0, 1, 1, 3};
			TS_ASSERT_EQUALS(collision::clipAxis(player, wall, 0, 10), 1.5);
			TS_ASSERT_EQUALS(collision::clipAxis(player, wall, 1, 10), 10);
			DoubleBox floor{-1, -1, -1, 3, 3, 1};
			TS_ASSERT_EQUALS(collision::clipAxis(player, floor, 2, -10), 0);
		}

		void testWorldCollisionAndRotation() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air");
			Block &wall = registry.registerBlock<Block>("default:wall");
			wall.setRotatable(true);
			wall.param().allocateBits(BlockParam::Rotation, 5);
			wall.getState(0).boundingBox({0, 0, 0, 0.25f, 1, 1});
			DoubleBox rotated = collision::blockBox(wall.getState(0), 1, 0, 0, 0);
			TS_ASSERT_DELTA(rotated.sizeX, 1, 0.0001);
			TS_ASSERT_DELTA(rotated.sizeY, 0.25, 0.0001);
			TestWorld world;
			world.chunk.setBlockRaw(2, 1, 1, wall.id());
			DoubleBox player{1, 1, 1, 0.5, 0.5, 1};
			Vector3d result = collision::move(world, player, {10, 0, 0});
			TS_ASSERT_DELTA(result.x, 0.5, 0.0001);
			wall.getState(0).isCollidable(false);
			result = collision::move(world, player, {10, 0, 0});
			TS_ASSERT_EQUALS(result.x, 10);
		}

		void testNegativePlayerChunk() {
			Player player;
			player.setPosition(-0.1, -16.1, -32.1);
			Vector3i chunk = player.getCurrentChunk();
			TS_ASSERT_EQUALS(chunk.x, -1);
			TS_ASSERT_EQUALS(chunk.y, -2);
			TS_ASSERT_EQUALS(chunk.z, -2);
		}

		void testReseedClearsHeightmap() {
			Heightmap reused{42};
			reused.getHighestBlockAt(-17, 20);
			reused.setSeed(-99);
			Heightmap fresh{-99};
			for (int x = -32; x < 32; ++x)
				TS_ASSERT_EQUALS(reused.getHighestBlockAt(x, 20), fresh.getHighestBlockAt(x, 20));
		}

};

#endif // WORLDSYSTEMSTESTS_HPP_
