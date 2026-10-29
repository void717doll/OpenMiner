#ifndef RETROTERRAINTESTS_HPP_
#define RETROTERRAINTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/RetroTerrain.hpp"

class RetroTerrainTests : public CxxTest::TestSuite {
	public:
		void testJavaNoiseOracle() {
			retro::Random random(0);
			retro::Octaves noise(random, 16);
			auto values = noise.sample(-8, 0, 12, 5, 1, 5, 684.412, 684.412, 684.412);
			TS_ASSERT_DELTA(values[0], 7016.8125461353757, 1e-9);
			TS_ASSERT_DELTA(values[1], 5537.280924962176, 1e-9);
			TS_ASSERT_DELTA(values[2], 4051.2006341602359, 1e-9);
		}

		void testTerrainDensityAndSeed() {
			retro::Terrain a(42), b(42), other(43);
			auto first = a.density(-1, 2), same = b.density(-1, 2), different = other.density(-1, 2);
			TS_ASSERT(first == same);
			TS_ASSERT(first != different);
			for (int x = 0; x < 16; ++x) {
				TS_ASSERT(retro::Terrain::sample(first, x, 0, -1) > 0);
				TS_ASSERT(retro::Terrain::sample(first, x, 0, 128) < 0);
			}
		}

		void testBetaClimateOracle() {
			retro::Climate climate(0);
			auto map = climate.sample(0, 0);
			TS_ASSERT_DELTA(map.temperature[0], 0.91868674455220250, 1e-14);
			TS_ASSERT_DELTA(map.humidity[0], 0.51627697992666610, 1e-14);
			TS_ASSERT_EQUALS(int(map.biomes[0]), int(retro::Biome::Forest));
			TS_ASSERT_EQUALS(int(retro::lookupBiome(0.05, 0.9)), int(retro::Biome::Tundra));
			TS_ASSERT_EQUALS(int(retro::lookupBiome(1, 0.1)), int(retro::Biome::Desert));
			TS_ASSERT_EQUALS(int(retro::lookupBiome(1, 1)), int(retro::Biome::Rainforest));
		}

		void testBetaSurfaceOracle() {
			const std::uint64_t expected[] = {
				0xebd5180d10a0dd38ULL, 0x4ba668733554f410ULL, 0xb6b13a50b1a3c3feULL,
				0xe5d746bae3d9f4e6ULL, 0x04cd4fbc6c001876ULL, 0xc41785bcb0975f53ULL,
				0xac1b9626f8d55035ULL, 0x7a825ed28a0e46fcULL, 0x644d12f35d1660faULL
			};
			std::size_t sample = 0;
			for (int seed : {0, 42, -99}) {
				retro::Terrain terrain(seed);
				for (auto position : {std::array<int, 2>{0, 0}, {-1, 2}, {17, -8}}) {
					auto blocks = terrain.surface(position[0], position[1]);
					std::uint64_t hash = 1469598103934665603ULL;
					for (auto block : blocks) hash = (hash ^ block) * 1099511628211ULL;
					TS_ASSERT_EQUALS(hash, expected[sample++]);
				}
			}
		}

};

#endif // RETROTERRAINTESTS_HPP_
