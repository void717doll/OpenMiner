#ifndef RETROCLIMATE_HPP_
#define RETROCLIMATE_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "common/world/RetroRandom.hpp"

namespace retro {

enum class Biome : std::uint8_t {
	Rainforest, Swampland, SeasonalForest, Forest, Savanna, Shrubland,
	Taiga, Desert, Plains, Tundra, IceDesert
};

inline const char *biomeName(Biome biome) {
	static constexpr const char *names[] = {"rainforest", "swampland", "seasonal_forest", "forest", "savanna", "shrubland", "taiga", "desert", "plains", "tundra", "ice_desert"};
	return names[unsigned(biome)];
}

inline Biome classifyBiome(float temperature, float humidity) {
	humidity *= temperature;
	if (temperature < 0.1f) return Biome::Tundra;
	if (humidity < 0.2f) {
		if (temperature < 0.5f) return Biome::Tundra;
		if (temperature < 0.95f) return Biome::Savanna;
		return Biome::Desert;
	}
	if (humidity > 0.5f && temperature < 0.7f) return Biome::Swampland;
	if (temperature < 0.5f) return Biome::Taiga;
	if (temperature < 0.97f) return humidity < 0.35f ? Biome::Shrubland : Biome::Forest;
	if (humidity < 0.45f) return Biome::Plains;
	return humidity < 0.9f ? Biome::SeasonalForest : Biome::Rainforest;
}

inline Biome lookupBiome(double temperature, double humidity) {
	int t = int(temperature * 63), h = int(humidity * 63);
	return classifyBiome(float(t) / 63.f, float(h) / 63.f);
}

class Simplex {
	public:
		explicit Simplex(Random &random) {
			m_x = random.nextDouble() * 256;
			m_y = random.nextDouble() * 256;
			random.nextDouble();
			for (int i = 0; i < 256; ++i) m_permutation[i] = i;
			for (int i = 0; i < 256; ++i) {
				std::swap(m_permutation[i], m_permutation[i + random.nextInt(256 - i)]);
				m_permutation[i + 256] = m_permutation[i];
			}
		}
		void add(std::vector<double> &output, double x, double y, int width, int depth,
		         double scaleX, double scaleY, double amplitude) const {
			const double skew = 0.5 * (std::sqrt(3.0) - 1), unskew = (3 - std::sqrt(3.0)) / 6;
			std::size_t index = 0;
			for (int ix = 0; ix < width; ++ix) {
				double px = (x + ix) * scaleX + m_x;
				for (int iy = 0; iy < depth; ++iy) {
					double py = (y + iy) * scaleY + m_y;
					double offset = (px + py) * skew;
					int gx = wrap(px + offset), gy = wrap(py + offset);
					double origin = double(gx + gy) * unskew;
					double dx = px - (gx - origin), dy = py - (gy - origin);
					int stepX = dx > dy ? 1 : 0, stepY = dx > dy ? 0 : 1;
					double midX = dx - stepX + unskew, midY = dy - stepY + unskew;
					double endX = dx - 1 + 2 * unskew, endY = dy - 1 + 2 * unskew;
					int hx = gx & 255, hy = gy & 255;
					double a = contribution(m_permutation[hx + m_permutation[hy]] % 12, dx, dy);
					double b = contribution(m_permutation[hx + stepX + m_permutation[hy + stepY]] % 12, midX, midY);
					double c = contribution(m_permutation[hx + 1 + m_permutation[hy + 1]] % 12, endX, endY);
					output[index++] += 70 * (a + b + c) * amplitude;
				}
			}
		}
	private:
		static int wrap(double value) { return value > 0 ? int(value) : int(value) - 1; }
		static double contribution(int hash, double x, double y) {
			static constexpr int gradients[12][2] = {{1,1},{-1,1},{1,-1},{-1,-1},{1,0},{-1,0},{1,0},{-1,0},{0,1},{0,-1},{0,1},{0,-1}};
			double weight = 0.5 - x * x - y * y;
			if (weight < 0) return 0;
			weight *= weight;
			return weight * weight * (gradients[hash][0] * x + gradients[hash][1] * y);
		}
		std::array<int, 512> m_permutation;
		double m_x, m_y;
};

class ClimateOctaves {
	public:
		ClimateOctaves(std::int64_t seed, int count) {
			Random random(seed);
			for (int i = 0; i < count; ++i) m_layers.emplace_back(random);
		}
		std::vector<double> sample(double x, double y, int width, int depth, double sx, double sy, double frequency) const {
			std::vector<double> result(std::size_t(width) * std::size_t(depth), 0);
			double scale = 1, weight = 1;
			for (const auto &layer : m_layers) {
				layer.add(result, x, y, width, depth, sx / 1.5 * scale, sy / 1.5 * scale, 0.55 / weight);
				scale *= frequency;
				weight *= 0.5;
			}
			return result;
		}
	private:
		std::vector<Simplex> m_layers;
};

struct ClimateMap {
	std::vector<double> temperature;
	std::vector<double> humidity;
	std::vector<Biome> biomes;
};

class Climate {
	public:
		explicit Climate(std::int64_t seed)
			: m_temperature(std::int64_t(std::uint64_t(seed) * 9871), 4),
			  m_humidity(std::int64_t(std::uint64_t(seed) * 39811), 4),
			  m_detail(std::int64_t(std::uint64_t(seed) * 543321), 2) {}

		ClimateMap sample(int x, int y, int width = 16, int depth = 16) const {
			ClimateMap map;
			map.temperature = m_temperature.sample(x, y, width, depth, double(0.025f), double(0.025f), 0.25);
			map.humidity = m_humidity.sample(x, y, width, depth, double(0.05f), double(0.05f), 1.0 / 3);
			auto detail = m_detail.sample(x, y, width, depth, 0.25, 0.25, 0.5882352941176471);
			map.biomes.reserve(map.temperature.size());
			for (std::size_t i = 0; i < map.temperature.size(); ++i) {
				double noise = detail[i] * 1.1 + 0.5;
				double temperature = (map.temperature[i] * 0.15 + 0.7) * 0.99 + noise * 0.01;
				double humidity = (map.humidity[i] * 0.15 + 0.5) * 0.998 + noise * 0.002;
				temperature = 1 - (1 - temperature) * (1 - temperature);
				map.temperature[i] = std::clamp(temperature, 0.0, 1.0);
				map.humidity[i] = std::clamp(humidity, 0.0, 1.0);
				map.biomes.push_back(lookupBiome(map.temperature[i], map.humidity[i]));
			}
			return map;
		}
	private:
		ClimateOctaves m_temperature, m_humidity, m_detail;
};

}

#endif // RETROCLIMATE_HPP_
