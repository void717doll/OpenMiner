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

};

#endif // RETROTERRAINTESTS_HPP_
