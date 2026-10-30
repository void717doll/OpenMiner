#ifndef WORLDWORKBUDGETTESTS_HPP_
#define WORLDWORKBUDGETTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/world/World.hpp"
#include "server/world/ServerBlock.hpp"

class WorldWorkBudgetTests : public CxxTest::TestSuite {
	class TestWorld : public World {
		public:
			Chunk *getChunk(int, int, int) const override { return nullptr; }
	};
	class TestChunk : public Chunk {
		public:
			TestChunk(World &world, int x) : Chunk(x, 0, 0, world) { setInitialized(true); }
			unsigned updates = 0, processes = 0;
			bool repeat = false;
			void update() override { ++updates; if (repeat) world().addChunkToUpdate(this); }
			void process() override { ++processes; }
	};
	public:
		void testBacklogIsDeferredWithoutLosingDeduplication() {
			TestWorld world;
			std::vector<std::unique_ptr<TestChunk>> chunks;
			for (int i = 0; i < 100; ++i) {
				chunks.emplace_back(std::make_unique<TestChunk>(world, i));
				TS_ASSERT(world.addChunkToUpdate(chunks.back().get()));
				TS_ASSERT(!world.addChunkToUpdate(chunks.back().get()));
				TS_ASSERT(world.addChunkToProcess(chunks.back().get()));
			}
			world.update();
			unsigned updates = 0, processes = 0;
			for (const auto &chunk : chunks) { updates += chunk->updates; processes += chunk->processes; }
			TS_ASSERT(updates > 0 && updates <= 16);
			TS_ASSERT(processes > 0 && processes <= 2);
			for (unsigned frame = 0; frame < 100; ++frame) world.update();
			for (const auto &chunk : chunks) { TS_ASSERT_EQUALS(chunk->updates, 1u); TS_ASSERT_EQUALS(chunk->processes, 1u); }
		}
		void testSelfReschedulingAndQueuedChunkRemoval() {
			TestWorld world;
			TestChunk repeated(world, 0); repeated.repeat = true;
			world.addChunkToUpdate(&repeated);
			world.addChunkToProcess(&repeated);
			world.update(); world.update();
			TS_ASSERT_EQUALS(repeated.updates, 2u);
			TS_ASSERT_EQUALS(repeated.processes, 1u);
			world.removeChunkFromQueues(&repeated);
			world.update();
			TS_ASSERT_EQUALS(repeated.updates, 2u);
			auto removed = std::make_unique<TestChunk>(world, 1);
			world.addChunkToUpdate(removed.get()); world.addChunkToProcess(removed.get());
			world.removeChunkFromQueues(removed.get()); removed.reset();
			world.update();
		}
		void testNewNeighborReceivesLightFromEachSharedFace() {
			Registry registry; Registry::setInstance(registry);
			auto &air = registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0);
			air.isOpaque(false); air.isCollidable(false);
			TestWorld world;
			for (unsigned side = 0; side < 6; ++side) {
				TestChunk source(world, 0), neighbor(world, 1);
				source.setSurroundingChunk(side, &neighbor);
				neighbor.setSurroundingChunk(side ^ 1, &source);
				int x = side == Chunk::West ? 0 : side == Chunk::East ? 15 : 8;
				int y = side == Chunk::South ? 0 : side == Chunk::North ? 15 : 8;
				int z = side == Chunk::Bottom ? 0 : side == Chunk::Top ? 31 : 16;
				source.lightmap().setLightDataRaw(x, y, z, 15);
				source.lightmap().refreshBoundaryLights(side);
				for (unsigned pass = 0; pass < 20; ++pass) {
					source.lightmap().updateLights();
					neighbor.lightmap().updateLights();
				}
				if (side < 2) x = 15 - x;
				else if (side < 4) y = 15 - y;
				else z = 31 - z;
				TS_ASSERT_EQUALS(neighbor.lightmap().getTorchlight(x, y, z), 14);
				world.removeChunkFromQueues(&source);
				world.removeChunkFromQueues(&neighbor);
			}
		}

		void testIncrementalLightingConvergesToUnboundedResult() {
			Registry registry; Registry::setInstance(registry);
			auto &air = registry.registerBlock<ServerBlock>(BLOCK_AIR).getState(0);
			air.isOpaque(false); air.isCollidable(false);
			TestWorld world;
			TestChunk reference(world, 0), incremental(world, 1);
			reference.lightmap().addTorchlight(8, 8, 16, 15);
			incremental.lightmap().addTorchlight(8, 8, 16, 15);
			reference.lightmap().updateLights(1000000);
			incremental.lightmap().updateLights(1);
			TS_ASSERT(incremental.lightmap().hasPendingUpdates());
			for (unsigned pass = 0; pass < 10000 && incremental.lightmap().hasPendingUpdates(); ++pass)
				incremental.lightmap().updateLights(32);
			TS_ASSERT(!incremental.lightmap().hasPendingUpdates());
			for (int z = 0; z < CHUNK_HEIGHT; ++z) for (int y = 0; y < CHUNK_DEPTH; ++y) for (int x = 0; x < CHUNK_WIDTH; ++x)
				TS_ASSERT_EQUALS(incremental.lightmap().getLightDataRaw(x, y, z), reference.lightmap().getLightDataRaw(x, y, z));
			reference.lightmap().removeTorchlight(8, 8, 16); incremental.lightmap().removeTorchlight(8, 8, 16);
			reference.lightmap().updateLights(1000000);
			for (unsigned pass = 0; pass < 10000 && incremental.lightmap().hasPendingUpdates(); ++pass)
				incremental.lightmap().updateLights(32);
			TS_ASSERT(!incremental.lightmap().hasPendingUpdates());
			for (int z = 0; z < CHUNK_HEIGHT; ++z) for (int y = 0; y < CHUNK_DEPTH; ++y) for (int x = 0; x < CHUNK_WIDTH; ++x)
				TS_ASSERT_EQUALS(incremental.lightmap().getLightDataRaw(x, y, z), reference.lightmap().getLightDataRaw(x, y, z));
		}
};

#endif // WORLDWORKBUDGETTESTS_HPP_
