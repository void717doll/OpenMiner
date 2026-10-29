#ifndef RETROCAVES_HPP_
#define RETROCAVES_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#include "common/world/RetroRandom.hpp"

namespace retro {

class Caves {
	public:
		using Column = std::array<std::uint8_t, 32768>;

		// use beta block ids and (local x * 16 + local horizontal y) * 128 + vertical z.
		static void carve(std::int64_t seed, int chunkX, int chunkY, Column &blocks, bool nether = false) {
			Caves generator(seed, nether);
			std::int64_t xMultiplier = generator.m_random.nextLong() / 2 * 2 + 1;
			std::int64_t yMultiplier = generator.m_random.nextLong() / 2 * 2 + 1;
			for (int dx = -8; dx <= 8; ++dx) {
				int originX = signed32(std::uint32_t(chunkX) + std::uint32_t(dx));
				for (int dy = -8; dy <= 8; ++dy) {
					int originY = signed32(std::uint32_t(chunkY) + std::uint32_t(dy));
					std::uint64_t mixed = (std::uint64_t(std::int64_t(originX)) * std::uint64_t(xMultiplier)
						+ std::uint64_t(std::int64_t(originY)) * std::uint64_t(yMultiplier)) ^ std::uint64_t(seed);
					generator.m_random.setSeed(signed64(mixed));
					generator.generateOrigin(originX, originY, chunkX, chunkY, blocks);
				}
			}
		}

	private:
		explicit Caves(std::int64_t seed, bool nether) : m_random(seed), m_nether(nether) {}

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
			int count = m_random.nextInt(m_random.nextInt(m_random.nextInt(m_nether ? 10 : 40) + 1) + 1);
			if (m_random.nextInt(m_nether ? 5 : 15) != 0) count = 0;
			for (int cave = 0; cave < count; ++cave) {
				double x = origin(originX, m_random.nextInt(16));
				double z = m_nether ? m_random.nextInt(128) : m_random.nextInt(m_random.nextInt(120) + 8);
				double y = origin(originY, m_random.nextInt(16));
				int tunnels = 1;
				if (m_random.nextInt(4) == 0) {
					float radius = 1.0f + m_random.nextFloat() * 6.0f;
					tunnel(chunkX, chunkY, blocks, x, z, y, radius, 0, 0, -1, -1, 0.5);
					tunnels += m_random.nextInt(4);
				}
				for (int branch = 0; branch < tunnels; ++branch) {
					float yaw = m_random.nextFloat() * float(pi) * 2.0f;
					float pitch = (m_random.nextFloat() - 0.5f) * 2.0f / 8.0f;
					float width = m_random.nextFloat() * 2.0f;
					width += m_random.nextFloat();
					tunnel(chunkX, chunkY, blocks, x, z, y, m_nether ? width * 2.0f : width, yaw, pitch, 0, 0, m_nether ? 0.5 : 1);
				}
			}
		}

		void tunnel(int chunkX, int chunkY, Column &blocks, double x, double z, double y,
		            float width, float yaw, float pitch, int step, int length, double verticalScale) {
			double centerX = origin(chunkX, 8), centerY = origin(chunkY, 8);
			float yawDrift = 0, pitchDrift = 0;
			Random random(m_random.nextLong());
			if (length <= 0) length = 112 - random.nextInt(28);
			bool room = step == -1;
			if (room) step = length / 2;
			int branchAt = random.nextInt(length / 2) + length / 4;
			bool gentlePitch = random.nextInt(6) == 0;
			for (; step < length; ++step) {
				double radius = 1.5 + double(sine(float(step) * float(pi) / float(length)) * width * 1.0f);
				double verticalRadius = radius * verticalScale;
				float horizontal = cosine(pitch);
				x += double(cosine(yaw) * horizontal);
				z += double(sine(pitch));
				y += double(sine(yaw) * horizontal);
				pitch *= gentlePitch ? 0.92f : 0.7f;
				pitch += pitchDrift * 0.1f;
				yaw += yawDrift * 0.1f;
				pitchDrift *= 0.9f;
				yawDrift *= 12.0f / 16.0f;
				float first = random.nextFloat(), second = random.nextFloat(), third = random.nextFloat();
				pitchDrift += (first - second) * third * 2.0f;
				first = random.nextFloat(); second = random.nextFloat(); third = random.nextFloat();
				yawDrift += (first - second) * third * 4.0f;
				if (!room && step == branchAt && width > 1.0f) {
					float childWidth = random.nextFloat() * 0.5f + 0.5f;
					tunnel(chunkX, chunkY, blocks, x, z, y, childWidth, yaw - float(pi) * 0.5f, pitch / 3.0f, step, length, 1);
					childWidth = random.nextFloat() * 0.5f + 0.5f;
					tunnel(chunkX, chunkY, blocks, x, z, y, childWidth, yaw + float(pi) * 0.5f, pitch / 3.0f, step, length, 1);
					return;
				}
				if (!room && random.nextInt(4) == 0) continue;
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
							if (m_nether ? (block == 10 || block == 11) : (block == 8 || block == 9)) wet = true;
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
							if (nz <= -0.7 || nx * nx + nz * nz + ny * ny >= 1) continue;
							std::uint8_t block = blocks[index];
							if (block == 2) grass = true;
							if (block != (m_nether ? 87 : 1) && block != 2 && block != 3) continue;
							if (m_nether) { blocks[index] = 0; continue; }
							if (iz < 10) blocks[index] = 10;
							else {
								blocks[index] = 0;
								if (grass && blocks[index - 1] == 3) blocks[index - 1] = 2;
							}
						}
					}
				}
				if (room) break;
			}
		}

		static constexpr double pi = 3.141592653589793;
		Random m_random;
		bool m_nether;
};

}

#endif // RETROCAVES_HPP_
