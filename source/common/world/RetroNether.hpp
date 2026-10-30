#ifndef RETRONETHER_HPP_
#define RETRONETHER_HPP_

#include "common/world/RetroTerrain.hpp"

namespace retro {

// the two beta glowstone generators use the same cluster algorithm.
template <typename ReadBlock, typename WriteBlock>
bool glowstoneCluster(Random &random, int x, int y, int z, ReadBlock read, WriteBlock write) {
	if (read(x, y, z) != 0 || read(x, y, z + 1) != 87) return false;
	write(x, y, z);
	for (int attempt = 0; attempt < 1500; ++attempt) {
		int px = x + random.nextInt(8);
		px -= random.nextInt(8);
		int pz = z - random.nextInt(12);
		int py = y + random.nextInt(8);
		py -= random.nextInt(8);
		if (read(px, py, pz) != 0) continue;
		int neighbors = (read(px - 1, py, pz) == 89) + (read(px + 1, py, pz) == 89)
			+ (read(px, py, pz - 1) == 89) + (read(px, py, pz + 1) == 89)
			+ (read(px, py - 1, pz) == 89) + (read(px, py + 1, pz) == 89);
		if (neighbors == 1) write(px, py, pz);
	}
	return true;
}

class NetherTerrain {
	public:
		using Blocks = Terrain::Blocks;
		explicit NetherTerrain(std::int64_t seed)
			: m_random(seed), m_low(m_random, 16), m_high(m_random, 16), m_blend(m_random, 8),
			  m_sand(m_random, 4), m_surface(m_random, 4), m_scale(m_random, 10), m_depth(m_random, 16) {}

		std::array<double, 425> density(int chunkX, int chunkY) const {
			int x = chunkX * 4, y = chunkY * 4;
			auto blend = m_blend.sample(x, 0, y, 5, 17, 5, 684.412 / 80, 2053.236 / 60, 684.412 / 80);
			auto low = m_low.sample(x, 0, y, 5, 17, 5, 684.412, 2053.236, 684.412);
			auto high = m_high.sample(x, 0, y, 5, 17, 5, 684.412, 2053.236, 684.412);
			std::array<double, 17> gradient;
			for (int z = 0; z < 17; ++z) {
				gradient[z] = std::cos(double(z) * 3.141592653589793 * 6 / 17) * 2;
				double edge = z > 8 ? 16 - z : z;
				if (edge < 4) {
					edge = 4 - edge;
					gradient[z] -= edge * edge * edge * 10;
				}
			}
			std::array<double, 425> result;
			for (int column = 0; column < 25; ++column) {
				for (int z = 0; z < 17; ++z) {
					int index = column * 17 + z;
					double t = (blend[index] / 10 + 1) / 2;
					double value;
					if (t < 0) value = low[index] / 512;
					else if (t > 1) value = high[index] / 512;
					else value = low[index] / 512 + (high[index] / 512 - low[index] / 512) * t;
					value -= gradient[z];
					if (z > 13) {
						double fade = double(float(z - 13) / 3.0f);
						value = value * (1 - fade) + -10 * fade;
					}
					result[index] = value;
				}
			}
			return result;
		}

		Blocks surface(int chunkX, int chunkY) const {
			return surface(chunkX, chunkY, density(chunkX, chunkY));
		}

		Blocks surface(int chunkX, int chunkY, const std::array<double, 425> &grid) const {
			Blocks blocks{};
			for (int gx = 0; gx < 4; ++gx) {
				for (int gy = 0; gy < 4; ++gy) {
					for (int gz = 0; gz < 16; ++gz) {
						double a = grid[(gx * 5 + gy) * 17 + gz];
						double b = grid[(gx * 5 + gy + 1) * 17 + gz];
						double c = grid[((gx + 1) * 5 + gy) * 17 + gz];
						double d = grid[((gx + 1) * 5 + gy + 1) * 17 + gz];
						double da = (grid[(gx * 5 + gy) * 17 + gz + 1] - a) * 0.125;
						double db = (grid[(gx * 5 + gy + 1) * 17 + gz + 1] - b) * 0.125;
						double dc = (grid[((gx + 1) * 5 + gy) * 17 + gz + 1] - c) * 0.125;
						double dd = (grid[((gx + 1) * 5 + gy + 1) * 17 + gz + 1] - d) * 0.125;
						for (int iz = 0; iz < 8; ++iz) {
							double left = a, right = b;
							double dxLeft = (c - a) * 0.25, dxRight = (d - b) * 0.25;
							for (int ix = 0; ix < 4; ++ix) {
								double value = left, dy = (right - left) * 0.25;
								for (int iy = 0; iy < 4; ++iy) {
									int x = gx * 4 + ix, y = gy * 4 + iy, z = gz * 8 + iz;
									std::uint8_t block = 0;
									if (z < 32) block = 11;
									if (value > 0) block = 87;
									blocks[(x * 16 + y) * 128 + z] = block;
									value += dy;
								}
								left += dxLeft; right += dxRight;
							}
							a += da; b += db; c += dc; d += dd;
						}
					}
				}
			}
			auto sand = m_sand.sample(chunkX * 16, chunkY * 16, 0, 16, 16, 1, 1.0 / 32, 1.0 / 32, 1);
			auto gravel = m_sand.sample(chunkX * 16, 109.0134, chunkY * 16, 16, 1, 16, 1.0 / 32, 1, 1.0 / 32);
			auto soil = m_surface.sample(chunkX * 16, chunkY * 16, 0, 16, 16, 1, 1.0 / 16, 1.0 / 16, 1.0 / 16);
			std::uint64_t surfaceSeed = std::uint64_t(std::int64_t(chunkX)) * 341873128712ULL + std::uint64_t(std::int64_t(chunkY)) * 132897987541ULL;
			Random random{std::int64_t(surfaceSeed)};
			for (int y = 0; y < 16; ++y) {
				for (int x = 0; x < 16; ++x) {
					int column = x * 16 + y;
					bool soulSand = sand[column] + random.nextDouble() * 0.2 > 0;
					bool gravelBeach = gravel[column] + random.nextDouble() * 0.2 > 0;
					int thickness = int(soil[column] / 3 + 3 + random.nextDouble() * 0.25);
					int remaining = -1;
					std::uint8_t top = 87, filler = 87;
					for (int z = 127; z >= 0; --z) {
						auto &block = blocks[column * 128 + z];
						if (z >= 127 - random.nextInt(5)) block = 7;
						else if (z <= random.nextInt(5)) block = 7;
						else if (block == 0) remaining = -1;
						else if (block == 87) {
							if (remaining == -1) {
								if (thickness <= 0) { top = 0; filler = 87; }
								else if (z >= 60 && z <= 65) {
									top = 87; filler = 87;
									if (gravelBeach) top = 13;
									if (soulSand) { top = 88; filler = 88; }
								}
								if (z < 64 && top == 0) top = 11;
								remaining = thickness;
								block = z >= 63 ? top : filler;
							}
							else if (remaining > 0) { --remaining; block = filler; }
						}
					}
				}
			}
			return blocks;
		}

	private:
		Random m_random;
		// the last two generators consume the source rng sequence; their samples do not affect nether density.
		Octaves m_low, m_high, m_blend, m_sand, m_surface, m_scale, m_depth;
};

}

#endif // RETRONETHER_HPP_
