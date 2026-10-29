#ifndef RETROCAVETESTS_HPP_
#define RETROCAVETESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/RetroCaves.hpp"

class RetroCaveTests : public CxxTest::TestSuite {
	public:
		void testJavaColumnOracle() {
			// hashes of complete columns produced by the supplied java generator.
			auto blocks = stone();
			retro::Caves::carve(0, -1, 2, blocks);
			TS_ASSERT_EQUALS(hash(blocks), 0x739af2809471ef8aULL);
			blocks = stone();
			retro::Caves::carve(42, 0, 0, blocks);
			TS_ASSERT_EQUALS(hash(blocks), 0x12f58a22574b8e06ULL);
			blocks = stone();
			retro::Caves::carve(-99, 17, -8, blocks);
			TS_ASSERT_EQUALS(hash(blocks), 0xbd5469cf9b80bd8aULL);
		}

		void testLavaAtDepth() {
			auto blocks = stone();
			retro::Caves::carve(9876543210123456LL, 0, 0, blocks);
			TS_ASSERT_EQUALS(hash(blocks), 0x71fdc32d56177506ULL);
			TS_ASSERT_EQUALS(std::count(blocks.begin(), blocks.end(), 10), 559);
			for (std::size_t i = 0; i < blocks.size(); ++i)
				if (blocks[i] == 10) TS_ASSERT(i % 128 <= 10);
		}

		void testGrassSurfaceOracle() {
			retro::Caves::Column blocks{};
			for (int x = 0; x < 16; ++x)
				for (int y = 0; y < 16; ++y)
					for (int z = 0; z <= 64; ++z)
						blocks[(x * 16 + y) * 128 + z] = std::uint8_t(z == 64 ? 2 : z >= 61 ? 3 : 1);
			retro::Caves::carve(42, -1, 2, blocks);
			TS_ASSERT_EQUALS(hash(blocks), 0x8ded3814ed0b46a1ULL);
			TS_ASSERT_EQUALS(std::count(blocks.begin(), blocks.end(), 2), 249);
		}

		void testWaterAndBedrockRemainIntact() {
			for (std::uint8_t block : {std::uint8_t(9), std::uint8_t(7)}) {
				retro::Caves::Column blocks;
				blocks.fill(block);
				retro::Caves::carve(9876543210123456LL, 0, 0, blocks);
				TS_ASSERT_EQUALS(std::count(blocks.begin(), blocks.end(), block), blocks.size());
			}
		}

		void testRequestOrderDoesNotChangeColumn() {
			auto first = stone(), other = stone(), repeated = stone();
			retro::Caves::carve(42, 0, 0, first);
			retro::Caves::carve(42, 1, 0, other);
			retro::Caves::carve(42, 0, 0, repeated);
			TS_ASSERT(first == repeated);
		}

	private:
		static retro::Caves::Column stone() {
			retro::Caves::Column blocks;
			blocks.fill(1);
			return blocks;
		}
		static std::uint64_t hash(const retro::Caves::Column &blocks) {
			std::uint64_t value = 14695981039346656037ULL;
			for (std::uint8_t block : blocks) value = (value ^ block) * 1099511628211ULL;
			return value;
		}
};

#endif // RETROCAVETESTS_HPP_
