#ifndef MODERNRAVINES_HPP_
#define MODERNRAVINES_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#include "common/world/RetroRandom.hpp"

namespace modern {

using retro::Random;
// modernmcp MapGenRavine, using the original 128-block terrain columns.

class Ravines {
	public:
		using Column = std::array<std::uint8_t, 32768>;

		// use beta block ids and (local x * 16 + local horizontal y) * 128 + vertical z.
		static void carve(std::int64_t seed, int chunkX, int chunkY, Column &blocks) {
			Ravines generator(seed);
			std::int64_t xMultiplier = generator.m_random.nextLong();
			std::int64_t yMultiplier = generator.m_random.nextLong();
			for (int dx = -8; dx <= 8; ++dx) {
				int originX = signed32(std::uint32_t(chunkX) + std::uint32_t(dx));
				for (int dy = -8; dy <= 8; ++dy) {
					int originY = signed32(std::uint32_t(chunkY) + std::uint32_t(dy));
					std::uint64_t mixed = (std::uint64_t(std::int64_t(originX)) * std::uint64_t(xMultiplier)
						^ std::uint64_t(std::int64_t(originY)) * std::uint64_t(yMultiplier)) ^ std::uint64_t(seed);
					generator.m_random.setSeed(signed64(mixed));
					generator.generateOrigin(originX, originY, chunkX, chunkY, blocks);
				}
			}
		}

	private:
		explicit Ravines(std::int64_t seed) : m_random(seed) {}

		static std::int32_t signed32(std::uint32_t bits) {
			std::int32_t result;
			std::memcpy(&result, &bits, sizeof(result));
			return result;
		}
		static std::int64_t signed64(std::uint64_t bits) {
			std::int64_t result;
			std::memcpy(&result, &bits, sizeof(result));
			return result;
		}
		static int origin(int chunk, int offset = 0) {
			return signed32(std::uint32_t(chunk) * 16u + std::uint32_t(offset));
		}
		static int floorInt(double value) {
			constexpr int low = std::numeric_limits<int>::min(), high = std::numeric_limits<int>::max();
			int result = value <= low ? low : value >= high ? high : int(value);
			if (value < result) result = signed32(std::uint32_t(result) - 1);
			return result;
		}
		static const std::array<float, 65536> &sineTable() {
			static const auto table = [] {
				std::array<float, 65536> values{};
				for (int i = 0; i < 65536; ++i)
					values[i] = float(std::sin(double(i) * pi * 2.0 / 65536.0));
				return values;
			}();
			return table;
		}
		static float sine(float angle) { return sineTable()[std::uint32_t(int(angle * 10430.378f)) & 65535]; }
		static float cosine(float angle) { return sineTable()[std::uint32_t(int(angle * 10430.378f + 16384.0f)) & 65535]; }

		void generateOrigin(int originX, int originY, int chunkX, int chunkY, Column &blocks) {
			if (m_random.nextInt(50) != 0) return;
			double x = origin(originX, m_random.nextInt(16));
			double z = m_random.nextInt(m_random.nextInt(40) + 8) + 20;
			double y = origin(originY, m_random.nextInt(16));
			float yaw = m_random.nextFloat() * float(pi) * 2.0f;
			float pitch = (m_random.nextFloat() - 0.5f) * 2.0f / 8.0f;
			float width = m_random.nextFloat() * 2.0f;
			width = (width + m_random.nextFloat()) * 2.0f;
			tunnel(chunkX, chunkY, blocks, x, z, y, width, yaw, pitch);
		}

		void tunnel(int chunkX, int chunkY, Column &blocks, double x, double z, double y,
		            float width, float yaw, float pitch) {
			double centerX = origin(chunkX, 8), centerY = origin(chunkY, 8);
			float yawDrift = 0, pitchDrift = 0;
			Random random(m_random.nextLong());
			int length = 112 - random.nextInt(28);
			std::array<float, 128> shape{};
			float scale = 1;
			for (int h = 0; h < 128; ++h) {
				if (h == 0 || random.nextInt(3) == 0) {
					float first = random.nextFloat();
					scale = 1 + first * random.nextFloat();
				}
				shape[h] = scale * scale;
			}
			for (int step = 0; step < length; ++step) {
				double radius = 1.5 + double(sine(float(step) * float(pi) / float(length)) * width * 1.0f);
				double verticalRadius = radius * 3;
				radius *= (double(random.nextFloat()) * 0.25 + 0.75);
				verticalRadius *= (double(random.nextFloat()) * 0.25 + 0.75);
				float horizontal = cosine(pitch);
				x += double(cosine(yaw) * horizontal);
				z += double(sine(pitch));
				y += double(sine(yaw) * horizontal);
				pitch *= 0.7f;
				pitch += pitchDrift * 0.05f;
				yaw += yawDrift * 0.05f;
				pitchDrift *= 0.8f;
				yawDrift *= 0.5f;
				float first = random.nextFloat(), second = random.nextFloat(), third = random.nextFloat();
				pitchDrift += (first - second) * third * 2.0f;
				first = random.nextFloat(); second = random.nextFloat(); third = random.nextFloat();
				yawDrift += (first - second) * third * 4.0f;
				if (random.nextInt(4) == 0) continue;
				double dx = x - centerX, dy = y - centerY, remaining = length - step;
				double reach = double(width + 2.0f + 16.0f);
				if (dx * dx + dy * dy - remaining * remaining > reach * reach) return;
				if (x < centerX - 16 - radius * 2 || y < centerY - 16 - radius * 2
				 || x > centerX + 16 + radius * 2 || y > centerY + 16 + radius * 2) continue;
				int minX = std::max(0, signed32(std::uint32_t(floorInt(x - radius)) - std::uint32_t(origin(chunkX)) - 1));
				int maxX = std::min(16, signed32(std::uint32_t(floorInt(x + radius)) - std::uint32_t(origin(chunkX)) + 1));
				int minY = std::max(0, signed32(std::uint32_t(floorInt(y - radius)) - std::uint32_t(origin(chunkY)) - 1));
				int maxY = std::min(16, signed32(std::uint32_t(floorInt(y + radius)) - std::uint32_t(origin(chunkY)) + 1));
				int minZ = std::max(1, floorInt(z - verticalRadius) - 1);
				int maxZ = std::min(120, floorInt(z + verticalRadius) + 1);
				bool wet = false;
				for (int ix = minX; !wet && ix < maxX; ++ix) {
					for (int iy = minY; !wet && iy < maxY; ++iy) {
						for (int iz = maxZ + 1; !wet && iz >= minZ - 1; --iz) {
							if (iz < 0 || iz >= 128) continue;
							std::uint8_t block = blocks[(ix * 16 + iy) * 128 + iz];
							if (block == 8 || block == 9) wet = true;
							if (iz != minZ - 1 && ix != minX && ix != maxX - 1 && iy != minY && iy != maxY - 1)
								iz = minZ;
						}
					}
				}
				if (wet) continue;
				for (int ix = minX; ix < maxX; ++ix) {
					double nx = (double(ix + origin(chunkX)) + 0.5 - x) / radius;
					for (int iy = minY; iy < maxY; ++iy) {
						double ny = (double(iy + origin(chunkY)) + 0.5 - y) / radius;
						int index = (ix * 16 + iy) * 128 + maxZ;
						bool grass = false;
						if (nx * nx + ny * ny >= 1) continue;
						for (int iz = maxZ - 1; iz >= minZ; --iz, --index) {
							double nz = (double(iz) + 0.5 - z) / verticalRadius;
							if ((nx * nx + ny * ny) * shape[iz] + nz * nz / 6 >= 1) continue;
							std::uint8_t block = blocks[index];
							if (block == 2) grass = true;
							if (block != 1 && block != 2 && block != 3) continue;
							if (iz < 10) blocks[index] = 10;
							else {
								blocks[index] = 0;
								if (grass && blocks[index - 1] == 3) blocks[index - 1] = 2;
							}
						}
					}
				}
			}
		}

		static constexpr double pi = 3.141592653589793;
		Random m_random;
};

}

#endif // MODERNRAVINES_HPP_
