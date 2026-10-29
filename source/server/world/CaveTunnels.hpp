#ifndef CAVETUNNELS_HPP_
#define CAVETUNNELS_HPP_

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace caves {

// replay nearby origins so carving never depends on chunk request order.
template<typename Carve>
void generate(std::int32_t seed, unsigned dimension, int bx, int by, int bz,
              int width, int depth, int height, Carve carve) {
	constexpr int regionSize = 64;
	auto region = [](int coordinate) {
		return int(std::floor(double(coordinate) / regionSize));
	};
	for (int rz = region(bz) - 2; rz <= region(bz + height - 1) + 2; ++rz) {
		for (int ry = region(by) - 2; ry <= region(by + depth - 1) + 2; ++ry) {
			for (int rx = region(bx) - 2; rx <= region(bx + width - 1) + 2; ++rx) {
				std::seed_seq sequence{std::uint32_t(seed), dimension, std::uint32_t(rx),
					std::uint32_t(ry), std::uint32_t(rz), 0x43415645u};
				std::mt19937 random(sequence);
				if (random() % 3 != 0)
					continue;
				auto unit = [&]() { return double(random()) / 4294967296.0; };
				double startX = double(rx) * regionSize + unit() * regionSize;
				double startY = double(ry) * regionSize + unit() * regionSize;
				double startZ = double(rz) * regionSize + unit() * regionSize;
				double heading = unit() * 6.283185307179586;
				for (int branch = 0; branch < 2; ++branch) {
					double x = startX, y = startY, z = startZ;
					double yaw = heading + branch * 1.2;
					double pitch = (unit() - 0.5) * 0.4;
					for (int step = 0; step < 48; ++step) {
						double radius = 2.0 + std::sin(step * 0.065) * 1.5;
						if (x + radius >= bx && x - radius < bx + width
						 && y + radius >= by && y - radius < by + depth
						 && z + radius >= bz && z - radius < bz + height) {
							for (int iz = std::max(0, int(std::floor(z - radius - bz))); iz < std::min(height, int(std::ceil(z + radius - bz))); ++iz) {
								for (int iy = std::max(0, int(std::floor(y - radius - by))); iy < std::min(depth, int(std::ceil(y + radius - by))); ++iy) {
									for (int ix = std::max(0, int(std::floor(x - radius - bx))); ix < std::min(width, int(std::ceil(x + radius - bx))); ++ix) {
										double dx = bx + ix + 0.5 - x;
										double dy = by + iy + 0.5 - y;
										double dz = bz + iz + 0.5 - z;
										if (dx * dx + dy * dy + dz * dz < radius * radius)
											carve(ix, iy, iz);
									}
								}
							}
						}
						yaw += (unit() - 0.5) * 0.35;
						pitch = pitch * 0.85 + (unit() - 0.5) * 0.12;
						x += std::cos(yaw) * 1.5;
						y += std::sin(yaw) * 1.5;
						z += pitch;
					}
				}
			}
		}
	}
}

}

#endif // CAVETUNNELS_HPP_
