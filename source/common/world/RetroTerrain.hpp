#ifndef RETROTERRAIN_HPP_
#define RETROTERRAIN_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace retro {

// java.util.Random's 48-bit generator, used by the supplied beta sources.
class Random {
	public:
		explicit Random(std::int64_t seed) : m_seed((std::uint64_t(seed) ^ 0x5deece66dULL) & mask) {}
		std::uint32_t next(unsigned bits) {
			m_seed = (m_seed * 0x5deece66dULL + 11) & mask;
			return std::uint32_t(m_seed >> (48 - bits));
		}
		int nextInt(int bound) {
			if (bound <= 0) throw std::invalid_argument("random bound must be positive");
			if ((bound & -bound) == bound)
				return int((std::uint64_t(bound) * next(31)) >> 31);
			std::uint32_t bits, value;
			do {
				bits = next(31);
				value = bits % unsigned(bound);
			} while (bits - value + unsigned(bound - 1) >= 0x80000000u);
			return int(value);
		}
		double nextDouble() {
			std::uint64_t high = next(26);
			return double((high << 27) + next(27)) / 9007199254740992.0;
		}
	private:
		static constexpr std::uint64_t mask = (1ULL << 48) - 1;
		std::uint64_t m_seed;
};

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
		std::vector<double> sample(int x, int y, int z, int width, int height, int depth,
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
		explicit Terrain(std::int64_t seed)
			: m_random(seed), m_low(m_random, 16), m_high(m_random, 16), m_blend(m_random, 8),
			  m_sand(m_random, 4), m_surface(m_random, 4), m_scale(m_random, 10), m_depth(m_random, 16) {}

		std::array<double, 425> density(int chunkX, int chunkY) const {
			int x = chunkX * 4, y = chunkY * 4;
			auto scale = m_scale.sample(x, 10, y, 5, 1, 5, 1.121, 1, 1.121);
			auto depth = m_depth.sample(x, 10, y, 5, 1, 5, 200, 1, 200);
			auto blend = m_blend.sample(x, 0, y, 5, 17, 5, 684.412 / 80, 684.412 / 160, 684.412 / 80);
			auto low = m_low.sample(x, 0, y, 5, 17, 5, 684.412, 684.412, 684.412);
			auto high = m_high.sample(x, 0, y, 5, 17, 5, 684.412, 684.412, 684.412);
			std::array<double, 425> result;
			for (int column = 0; column < 25; ++column) {
				// oldworld still selects biomes separately; use temperate climate for density.
				double climate = 1 - std::pow(1 - 0.5 * 0.5, 4);
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
		Random m_random;
		Octaves m_low, m_high, m_blend, m_sand, m_surface, m_scale, m_depth;
};

}

#endif // RETROTERRAIN_HPP_
