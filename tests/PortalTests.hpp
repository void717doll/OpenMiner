#ifndef PORTALTESTS_HPP_
#define PORTALTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/world/Dimension.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/ServerBlock.hpp"
#include "server/world/ServerWorld.hpp"

class PortalTests : public CxxTest::TestSuite {
	struct Fixture {
		Registry definitions;
		PlayerList players;
		Dimension dimension{0, "test:portals", "portals"};
		std::unique_ptr<ServerWorld> world;
		u16 obsidian, portal;
		Fixture() {
			Registry::setInstance(definitions);
			definitions.registerBlock<ServerBlock>(BLOCK_AIR).getState(0).isCollidable(false);
			obsidian = definitions.registerBlock<ServerBlock>("default:obsidian").id();
			auto &block = definitions.registerBlock<ServerBlock>("default:portal");
			portal = block.id(); block.addState(); block.param().allocateBits(BlockParam::State, 1);
			world = std::make_unique<ServerWorld>(players, dimension, 42);
			world->getOrCreateChunk(0,0,0).setInitialized(true);
		}
		void build(Vector3i origin, Vector3i axis) {
			for (int h=-1;h<=2;++h) for(int z=-1;z<=3;++z) {
				bool side=h==-1||h==2, end=z==-1||z==3;
				if (side==end) continue;
				Vector3i point=origin+axis*h+Vector3i{0,0,z};
				world->setBlock(point.x,point.y,point.z,obsidian);
			}
		}
	};
public:
	void testBothFrameOrientationsAndOptionalCorners() {
		for (Vector3i direction : {Vector3i{1,0,0},Vector3i{0,1,0}}) {
			Fixture f; Vector3i base{5,5,5}, origin,axis; f.build(base,direction);
			TS_ASSERT(f.world->ignitePortal(base+Vector3i{0,0,-1}));
			TS_ASSERT(f.world->portalFrameAt(base+direction+Vector3i{0,0,2},origin,axis));
			TS_ASSERT_EQUALS(origin,base); TS_ASSERT_EQUALS(axis,direction);
			TS_ASSERT(!f.world->ignitePortal(base));
			f.world->setBlock(5,5,4,0); f.world->validatePortal(5,5,5);
			TS_ASSERT_EQUALS(f.world->getBlock(5,5,5),0);
		}
	}
	void testIncompleteOrObstructedFramesCannotIgnite() {
		Fixture f; Vector3i base{5,5,5}; f.build(base,{1,0,0});
		f.world->setBlock(5,5,6,f.obsidian);
		TS_ASSERT(!f.world->ignitePortal(base));
		f.world->setBlock(5,5,6,0); f.world->setBlock(4,5,5,0);
		TS_ASSERT(!f.world->ignitePortal(base));
	}
	void testMetadataTypeCheckAndPortalCooldown() {
		BlockMetadata data; data.setString("portal_x","wrong");
		TS_ASSERT(!data.hasInt("portal_x")); data.setInt("portal_x",8); TS_ASSERT(data.hasInt("portal_x"));
		ServerPlayer player("test",false);
		TS_ASSERT(player.tryPortal(100)); TS_ASSERT(!player.tryPortal(101)); TS_ASSERT(player.tryPortal(1600));
	}
};
#endif
