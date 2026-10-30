#ifndef WORLDCOLLISIONTESTS_HPP_
#define WORLDCOLLISIONTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/world/WorldCollision.hpp"

class WorldCollisionTests : public CxxTest::TestSuite {
	class TestChunk : public Chunk {
		public:
			TestChunk(World &world) : Chunk(0, 0, 0, world) { setInitialized(true); }
			void update() override {}
			void process() override {}
	};

	class TestWorld : public World {
		public:
			TestWorld() : chunk(*this) {}
			Chunk *getChunk(int x, int y, int z) const override {
				return loaded && x == 0 && y == 0 && z == 0 ? const_cast<TestChunk *>(&chunk) : nullptr;
			}
			void appendCollisionBoxes(const DoubleBox &moving, const DoubleBox &broadphase,
				std::vector<DoubleBox> &boxes) const override {
				++objectQueries;
				for (const auto &box : objects) {
					if (box.position() == moving.position() && box.size() == moving.size())
						continue;
					if (box.intersects(broadphase))
						boxes.push_back(box);
				}
			}
			TestChunk chunk;
			bool loaded = true;
			std::vector<DoubleBox> objects;
			mutable unsigned objectQueries = 0;
	};

	public:
		void testWorldGroupsAndSupportingNode() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			auto &floor = registry.registerBlock<Block>("test:floor");
			floor.addGroup("group:bouncy", u16(-80));
			floor.addGroup("group:slippery", 4);
			TestWorld world;
			world.chunk.setBlockRaw(8, 8, 0, floor.id());
			auto support = collision::step(world, {8.1, 8.1, 1, 0.6, 0.6, 1.8}, {0, 0, 0}, {0, 0, 0}, 0.05);
			TS_ASSERT(support.grounded && support.hasSupport);
			TS_ASSERT_EQUALS(support.supportNode.x, 8);
			TS_ASSERT_EQUALS(support.supportNode.y, 8);
			TS_ASSERT_EQUALS(support.supportNode.z, 0);
			TS_ASSERT_EQUALS(support.bouncy, -80);
			TS_ASSERT_EQUALS(support.slippery, 4);
			auto bounce = collision::step(world, {8.1, 8.1, 2, 0.6, 0.6, 1.8}, {0, 0, -10}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(bounce.velocity.z, 8, 1e-9);
		}

		void testUnloadedWorldAndBoundaryStaySolid() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			TestWorld world;
			auto boundary = collision::step(world, {14, 8, 8, 0.6, 0.6, 1.8}, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(boundary.movement.x, 1.4, 1e-9);
			TS_ASSERT(boundary.unloaded);
			world.chunk.setInitialized(false);
			auto uninitialized = collision::step(world, {8, 8, 8, 0.6, 0.6, 1.8}, {20, 0, 0}, {0, 0, -10}, 0.2);
			TS_ASSERT(uninitialized.movement.isZero() && uninitialized.unloaded);
			world.loaded = false;
			auto frozen = collision::step(world, {8, 8, 8, 0.6, 0.6, 1.8}, {20, 0, 0}, {0, 0, -10}, 0.2);
			TS_ASSERT(frozen.movement.isZero() && frozen.velocity.isZero() && frozen.unloaded);
		}

		void testRotatedPartialBlockUsesItsActualBox() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			auto &slab = registry.registerBlock<Block>("test:slab");
			slab.setRotatable(true);
			slab.param().allocateBits(BlockParam::Rotation, 5);
			slab.getState(0).boundingBox({0, 0, 0, 1, 1, 0.5});
			TestWorld world;
			world.chunk.setBlockRaw(8, 8, 8, slab.id());
			world.chunk.setData(8, 8, 8, slab.param().setParam(BlockParam::Rotation, 0, 16));
			auto box = collision::blockBox(slab.getState(0), world.getData(8, 8, 8), 8, 8, 8);
			TS_ASSERT_DELTA(box.sizeX, 0.5, 1e-9);
			TS_ASSERT_DELTA(box.sizeZ, 1, 1e-9);
			auto result = collision::step(world, {6, 8.1, 8.1, 0.6, 0.6, 0.6}, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(result.movement.x, box.x - 6.6, 1e-8);
		}

		void testObjectCollisionAndOptionalCollection() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			TestWorld world;
			DoubleBox body{8, 8, 8, 0.6, 0.6, 1.8};
			world.objects = {body, DoubleBox{10, 8, 8, 0.6, 0.6, 1.8}};
			auto blocked = collision::step(world, body, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(blocked.movement.x, 1.4, 1e-9);
			TS_ASSERT(blocked.horizontalCollision && !blocked.unloaded);
			TS_ASSERT_EQUALS(world.objectQueries, 1u);
			auto through = collision::step(world, body, {20, 0, 0}, {0, 0, 0}, 0.2, 0, false);
			TS_ASSERT_DELTA(through.movement.x, 4, 1e-9);
			TS_ASSERT_EQUALS(world.objectQueries, 1u);
			TS_ASSERT_DELTA(collision::move(world, body, {4, 0, 0}).x, 1.4, 1e-9);
			TS_ASSERT_DELTA(collision::move(world, body, {4, 0, 0}, false).x, 4, 1e-9);
		}

		void testStandingOnObjectHasNoBlockSupportNode() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			TestWorld world;
			world.objects = {DoubleBox{8, 8, 7, 1, 1, 1}};
			auto result = collision::step(world, {8.1, 8.1, 8, 0.6, 0.6, 1.8}, {0, 0, -1}, {0, 0, 0}, 0.05);
			TS_ASSERT(result.grounded && result.standingOnObject);
			TS_ASSERT(!result.hasSupport);
			TS_ASSERT_DELTA(result.velocity.z, 0, 1e-9);
		}

		void testNearestObjectOrUnloadedBoundaryWins() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			TestWorld world;
			DoubleBox body{14, 8, 8, 0.6, 0.6, 1.8};
			world.objects = {DoubleBox{17, 8, 8, 0.6, 0.6, 1.8}};
			auto boundary = collision::step(world, body, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(boundary.movement.x, 1.4, 1e-9);
			TS_ASSERT(boundary.unloaded);
			world.objects = {DoubleBox{15, 8, 8, 0.6, 0.6, 1.8}};
			auto object = collision::step(world, body, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(object.movement.x, 0.4, 1e-9);
			TS_ASSERT(!object.unloaded);
			world.loaded = false;
			world.objectQueries = 0;
			auto frozen = collision::step(world, body, {20, 0, 0}, {0, 0, 0}, 0.2);
			TS_ASSERT(frozen.movement.isZero() && frozen.unloaded);
			TS_ASSERT_EQUALS(world.objectQueries, 0u);
		}

		void testDisplacementWrapperKeepsSmallMovesAndFullDistance() {
			Registry registry;
			Registry::setInstance(registry);
			registry.registerBlock<Block>("air").getState(0).isCollidable(false);
			TestWorld world;
			Vector3d requested{1e-7, 2, -0.000003};
			auto movement = collision::move(world, {8, 8, 8, 0.6, 0.6, 1.8}, requested);
			TS_ASSERT_EQUALS(movement.x, requested.x);
			TS_ASSERT_EQUALS(movement.y, requested.y);
			TS_ASSERT_EQUALS(movement.z, requested.z);
		}
};

#endif // WORLDCOLLISIONTESTS_HPP_
