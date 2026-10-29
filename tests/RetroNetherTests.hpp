#ifndef RETRONETHERTESTS_HPP_
#define RETRONETHERTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/Heightmap.hpp"
#include "common/world/RetroCaves.hpp"
#include "common/world/RetroNether.hpp"

class RetroNetherTests : public CxxTest::TestSuite {
	public:
		void testJavaSurfaceAndCaveOracles() {
			const std::uint64_t surfaceHashes[] = {
				0x18fdb0ce7255159fULL, 0x6ff6188a053793f2ULL, 0x346dae171081c7c6ULL, 0x83cb08bcf3b720faULL,
				0x96c522c2f19247a9ULL, 0xd2616f086e399b38ULL, 0x880f0dae31c4f931ULL, 0xbba3dc64d972034cULL,
				0xec2a2ddb5b332077ULL, 0x3787fd81b09ba435ULL, 0xd4a06f59bac1aa55ULL, 0x5d6be4abe790b30bULL,
				0xbe82fbb987c86732ULL, 0x238508edbb3bd216ULL, 0x9e393731a4c309a4ULL, 0x35b00d3ab922e410ULL
			};
			const std::uint64_t caveHashes[] = {
				0x937be707f745634aULL, 0x6b240a4602aa44f8ULL, 0x87446a76050c7504ULL, 0xa11bc04c597b93faULL,
				0xa453b4eef9af6d51ULL, 0x63149295a4f5a293ULL, 0x7b482186e65dc73dULL, 0xbba3dc64d972034cULL,
				0xec2a2ddb5b332077ULL, 0x3787fd81b09ba435ULL, 0x330ea73ae3e96259ULL, 0x1cd4c7d80737fa27ULL,
				0xbe82fbb987c86732ULL, 0x238508edbb3bd216ULL, 0x05fba70970ed14dbULL, 0x679da15a59d9475aULL
			};
			std::size_t sample = 0;
			for (std::int64_t seed : {0LL, 42LL, -99LL, 9876543210123456LL}) {
				retro::NetherTerrain terrain(seed);
				for (auto position : {std::array<int, 2>{0, 0}, {-1, 2}, {17, -8}, {-101, 99}}) {
					auto blocks = terrain.surface(position[0], position[1]);
					TS_ASSERT_EQUALS(hash(blocks), surfaceHashes[sample]);
					retro::Caves::carve(seed, position[0], position[1], blocks, true);
					TS_ASSERT_EQUALS(hash(blocks), caveHashes[sample++]);
					for (int column = 0; column < 256; ++column) {
						TS_ASSERT_EQUALS(blocks[column * 128], 7);
						TS_ASSERT_EQUALS(blocks[column * 128 + 127], 7);
					}
				}
			}
		}

		void testNetherHeightmapReseed() {
			Heightmap heightmap(0, true);
			TS_ASSERT(heightmap.isNether());
			TS_ASSERT_EQUALS(hash(heightmap.getOrCreateChunk(0, 0).blocks()), 0x937be707f745634aULL);
			heightmap.setSeed(42);
			TS_ASSERT(heightmap.isNether());
			TS_ASSERT_EQUALS(hash(heightmap.getOrCreateChunk(0, 0).blocks()), 0xa453b4eef9af6d51ULL);
			TS_ASSERT_EQUALS(heightmap.getHighestBlockAt(0, 0), 127);
			TS_ASSERT_EQUALS(heightmap.getOrCreateChunk(0, 0).blockAt(0, 0, 128), 0);
		}

		void testNetherCavesPreserveLavaAndBedrock() {
			for (std::uint8_t material : {std::uint8_t(7), std::uint8_t(10), std::uint8_t(11)}) {
				retro::Caves::Column blocks;
				blocks.fill(material);
				retro::Caves::carve(42, 0, 0, blocks, true);
				TS_ASSERT_EQUALS(std::count(blocks.begin(), blocks.end(), material), blocks.size());
			}
		}

	private:
		static std::uint64_t hash(const retro::Caves::Column &blocks) {
			std::uint64_t value = 14695981039346656037ULL;
			for (std::uint8_t block : blocks) value = (value ^ block) * 1099511628211ULL;
			return value;
		}
};

#endif // RETRONETHERTESTS_HPP_
