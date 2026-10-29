#ifndef RETROTERRAIN_HPP_
#define RETROTERRAIN_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "common/world/RetroRandom.hpp"
#include "common/world/RetroClimate.hpp"

namespace retro {

class Perlin {
	public:
		explicit Perlin(Random &random) {
			m_x = random.nextDouble() * 256;
			m_y = random.nextDouble() * 256;
			m_z = random.nextDouble() * 256;
			for (int i = 0; i < 256; ++i) m_permutation[i] = i;
			for (int i = 0; i < 256; ++i) {
				std::swap(m_permutation[i], m_permutation[i + random.nextInt(256 - i)]);
				m_permutation[i + 256] = m_permutation[i];
			}
		}

		void add(std::vector<double> &output, double x, double y, double z,
		         int width, int height, int depth, double sx, double sy, double sz, double amplitude) const {
			std::size_t index = 0;
			int lastY = -1;
			double g00 = 0, g10 = 0, g01 = 0, g11 = 0;
			for (int ix = 0; ix < width; ++ix) {
				double px = (x + ix) * sx + m_x;
				int cellX = javaFloor(px); px -= cellX;
				int hx = cellX & 255;
				double fx = fade(px);
				for (int iz = 0; iz < depth; ++iz) {
					double pz = (z + iz) * sz + m_z;
					int cellZ = javaFloor(pz); pz -= cellZ;
					int hz = cellZ & 255;
					double fz = fade(pz);
					if (height == 1) {
						int a = m_permutation[m_permutation[hx]] + hz;
						int b = m_permutation[m_permutation[hx + 1]] + hz;
						double low = lerp(fx, grad2(m_permutation[a], px, pz), grad(m_permutation[b], px - 1, 0, pz));
						double high = lerp(fx, grad(m_permutation[a + 1], px, 0, pz - 1), grad(m_permutation[b + 1], px - 1, 0, pz - 1));
						output[index++] += lerp(fz, low, high) / amplitude;
						continue;
					}
					for (int iy = 0; iy < height; ++iy) {
						double py = (y + iy) * sy + m_y;
						int cellY = javaFloor(py); py -= cellY;
						int hy = cellY & 255;
						// retain the beta bulk sampler's vertical gradient caching.
						if (iy == 0 || hy != lastY) {
							lastY = hy;
							int a = m_permutation[hx] + hy, b = m_permutation[hx + 1] + hy;
							int aa = m_permutation[a] + hz, ab = m_permutation[a + 1] + hz;
							int ba = m_permutation[b] + hz, bb = m_permutation[b + 1] + hz;
							g00 = lerp(fx, grad(m_permutation[aa], px, py, pz), grad(m_permutation[ba], px - 1, py, pz));
							g10 = lerp(fx, grad(m_permutation[ab], px, py - 1, pz), grad(m_permutation[bb], px - 1, py - 1, pz));
							g01 = lerp(fx, grad(m_permutation[aa + 1], px, py, pz - 1), grad(m_permutation[ba + 1], px - 1, py, pz - 1));
							g11 = lerp(fx, grad(m_permutation[ab + 1], px, py - 1, pz - 1), grad(m_permutation[bb + 1], px - 1, py - 1, pz - 1));
						}
						output[index++] += lerp(fz, lerp(fade(py), g00, g10), lerp(fade(py), g01, g11)) / amplitude;
					}
				}
			}
		}
		static double lerp(double t, double a, double b) { return a + t * (b - a); }
	private:
		static int javaFloor(double value) {
			constexpr int low = std::numeric_limits<int>::min(), high = std::numeric_limits<int>::max();
			int cell = value <= low ? low : value >= high ? high : int(value);
			if (value < cell) cell = cell == low ? high : cell - 1;
			return cell;
		}
		static double fade(double x) { return x * x * x * (x * (x * 6 - 15) + 10); }
		static double grad(int hash, double x, double y, double z) {
			int h = hash & 15;
			double u = h < 8 ? x : y;
			double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
			return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
		}
		static double grad2(int hash, double x, double z) {
			int h = hash & 15;
			double u = (1 - ((h & 8) >> 3)) * x;
			double v = h < 4 ? 0 : (h == 12 || h == 14 ? x : z);
			return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
		}
		std::array<int, 512> m_permutation;
		double m_x, m_y, m_z;
};

class Octaves {
	public:
		Octaves(Random &random, int count) { for (int i = 0; i < count; ++i) m_layers.emplace_back(random); }
		std::vector<double> sample(double x, double y, double z, int width, int height, int depth,
		                           double sx, double sy, double sz) const {
			std::vector<double> result(width * height * depth, 0);
			double scale = 1;
			for (const auto &layer : m_layers) {
				layer.add(result, x, y, z, width, height, depth, sx * scale, sy * scale, sz * scale, scale);
				scale /= 2;
			}
			return result;
		}
	private:
		std::vector<Perlin> m_layers;
};

class Terrain {
	public:
		using Blocks = std::array<std::uint8_t, 32768>;
		explicit Terrain(std::int64_t seed)
			: m_seed(seed), m_climate(seed), m_random(seed), m_low(m_random, 16), m_high(m_random, 16), m_blend(m_random, 8),
			  m_sand(m_random, 4), m_surface(m_random, 4), m_scale(m_random, 10), m_depth(m_random, 16) {}

		std::int64_t seed() const { return m_seed; }
		const Climate &climate() const { return m_climate; }

		std::array<double, 425> density(int chunkX, int chunkY) const {
			return density(chunkX, chunkY, m_climate.sample(chunkX * 16, chunkY * 16));
		}

		std::array<double, 425> density(int chunkX, int chunkY, const ClimateMap &climateMap) const {
			int x = chunkX * 4, y = chunkY * 4;
			auto scale = m_scale.sample(x, 10, y, 5, 1, 5, 1.121, 1, 1.121);
			auto depth = m_depth.sample(x, 10, y, 5, 1, 5, 200, 1, 200);
			auto blend = m_blend.sample(x, 0, y, 5, 17, 5, 684.412 / 80, 684.412 / 160, 684.412 / 80);
			auto low = m_low.sample(x, 0, y, 5, 17, 5, 684.412, 684.412, 684.412);
			auto high = m_high.sample(x, 0, y, 5, 17, 5, 684.412, 684.412, 684.412);
			std::array<double, 425> result;
			for (int column = 0; column < 25; ++column) {
				int sampleIndex = ((column / 5) * 3 + 1) * 16 + (column % 5) * 3 + 1;
				double climate = climateMap.humidity[sampleIndex] * climateMap.temperature[sampleIndex];
				climate = 1 - climate;
				climate *= climate;
				climate *= climate;
				climate = 1 - climate;
				double stretch = std::min(1.0, (scale[column] + 256) / 512 * climate);
				double offset = depth[column] / 8000;
				if (offset < 0) offset = -offset * 0.3;
				offset = offset * 3 - 2;
				if (offset < 0) {
					offset = std::max(-1.0, offset / 2) / 1.4 / 2;
					stretch = 0;
				}
				else offset = std::min(1.0, offset) / 8;
				stretch = std::max(0.0, stretch) + 0.5;
				double center = 17.0 / 2 + offset * 17.0 / 16 * 4;
				for (int z = 0; z < 17; ++z) {
					int index = column * 17 + z;
					double gradient = (z - center) * 12 / stretch;
					if (gradient < 0) gradient *= 4;
					double t = std::clamp((blend[index] / 10 + 1) / 2, 0.0, 1.0);
					double value = Perlin::lerp(t, low[index] / 512, high[index] / 512) - gradient;
					if (z > 13) value = Perlin::lerp(double(float(z - 13) / 3.0f), value, -10);
					result[index] = value;
				}
			}
			return result;
		}

		Blocks surface(int chunkX, int chunkY) const {
			auto climateMap = m_climate.sample(chunkX * 16, chunkY * 16);
			auto grid = density(chunkX, chunkY, climateMap);
			return surface(chunkX, chunkY, climateMap, grid);
		}

		Blocks surface(int chunkX, int chunkY, const ClimateMap &climateMap, const std::array<double, 425> &grid) const {
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
									if (z < 64) block = climateMap.temperature[x * 16 + y] < 0.5 && z >= 63 ? 79 : 9;
									if (value > 0) block = 1;
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
			auto gravel = sampleGravel(chunkX, chunkY);
			auto soil = m_surface.sample(chunkX * 16, chunkY * 16, 0, 16, 16, 1, 1.0 / 16, 1.0 / 16, 1.0 / 16);
			std::uint64_t surfaceSeed = std::uint64_t(std::int64_t(chunkX)) * 341873128712ULL + std::uint64_t(std::int64_t(chunkY)) * 132897987541ULL;
			Random random{std::int64_t(surfaceSeed)};
			for (int y = 0; y < 16; ++y) {
				for (int x = 0; x < 16; ++x) {
					int column = x * 16 + y;
					Biome biome = climateMap.biomes[column];
					std::uint8_t biomeTop = biome == Biome::Desert || biome == Biome::IceDesert ? 12 : 2;
					std::uint8_t biomeFiller = biomeTop == 12 ? 12 : 3;
					bool sandBeach = sand[column] + random.nextDouble() * 0.2 > 0;
					bool gravelBeach = gravel[column] + random.nextDouble() * 0.2 > 3;
					int thickness = int(soil[column] / 3 + 3 + random.nextDouble() * 0.25);
					int remaining = -1;
					std::uint8_t top = biomeTop, filler = biomeFiller;
					for (int z = 127; z >= 0; --z) {
						auto &block = blocks[column * 128 + z];
						if (z <= random.nextInt(5)) block = 7;
						else if (block == 0) remaining = -1;
						else if (block == 1) {
							if (remaining == -1) {
								if (thickness <= 0) { top = 0; filler = 1; }
								else if (z >= 60 && z <= 65) {
									top = biomeTop; filler = biomeFiller;
									if (gravelBeach) { top = 0; filler = 13; }
									if (sandBeach) { top = 12; filler = 12; }
								}
								if (z < 64 && top == 0) top = 9;
								remaining = thickness;
								block = z >= 63 ? top : filler;
							}
							else if (remaining > 0) {
								--remaining;
								block = filler;
								if (remaining == 0 && filler == 12) { remaining = random.nextInt(4); filler = 24; }
							}
						}
					}
				}
			}
			return blocks;
		}

		static double sample(const std::array<double, 425> &density, int x, int y, int z) {
			if (z < 0) return 1;
			if (z >= 128) return -1;
			int gx = x / 4, gy = y / 4, gz = z / 8;
			double fx = (x % 4) / 4.0, fy = (y % 4) / 4.0, fz = (z % 8) / 8.0;
			auto at = [&](int dx, int dy) {
				int i = ((gx + dx) * 5 + gy + dy) * 17 + gz;
				return Perlin::lerp(fz, density[i], density[i + 1]);
			};
			return Perlin::lerp(fy, Perlin::lerp(fx, at(0, 0), at(1, 0)), Perlin::lerp(fx, at(0, 1), at(1, 1)));
		}
	private:
		std::vector<double> sampleGravel(int chunkX, int chunkY) const {
			return m_sand.sample(chunkX * 16, 109.0134, chunkY * 16, 16, 1, 16, 1.0 / 32, 1, 1.0 / 32);
		}
		std::int64_t m_seed;
		Climate m_climate;
		Random m_random;
		Octaves m_low, m_high, m_blend, m_sand, m_surface, m_scale, m_depth;
};

}

#endif // RETROTERRAIN_HPP_
