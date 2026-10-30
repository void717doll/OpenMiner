#ifndef BETATREESHAPE_HPP_
#define BETATREESHAPE_HPP_

#include <algorithm>
#include <cmath>
#include <vector>
#include "common/core/Vector3.hpp"
#include "common/world/RetroRandom.hpp"

namespace retro {
struct TreeCell { Vector3i position; bool log; };

// retromcp WorldGenForest and WorldGenTaiga2, with z as the vertical axis.
inline std::vector<TreeCell> betaTreeShape(Random &random, bool spruce) {
	std::vector<TreeCell> cells;
	int height = random.nextInt(spruce ? 4 : 3) + (spruce ? 6 : 5);
	if (spruce) {
		int bare = 1 + random.nextInt(2), crown = height - bare, maxRadius = 2 + random.nextInt(2);
		int radius = random.nextInt(2), limit = 1, reset = 0;
		for (int layer = 0; layer <= crown; ++layer) {
			for (int x = -radius; x <= radius; ++x) for (int y = -radius; y <= radius; ++y)
				if (std::abs(x) != radius || std::abs(y) != radius || radius == 0)
					cells.push_back({{x, y, height - layer}, false});
			if (radius >= limit) { radius = reset; reset = 1; limit = std::min(limit + 1, maxRadius); }
			else ++radius;
		}
		int shorten = random.nextInt(3);
		for (int z = 0; z < height - shorten; ++z) cells.push_back({{0, 0, z}, true});
	}
	else {
		for (int z = height - 3; z <= height; ++z) {
			int dz = z - height, radius = 1 - dz / 2;
			for (int x = -radius; x <= radius; ++x) for (int y = -radius; y <= radius; ++y)
				if (std::abs(x) != radius || std::abs(y) != radius || (random.nextInt(2) != 0 && dz != 0))
					cells.push_back({{x, y, z}, false});
		}
		for (int z = 0; z < height; ++z) cells.push_back({{0, 0, z}, true});
	}
	return cells;
}
}
#endif
