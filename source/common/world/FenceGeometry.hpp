#ifndef FENCEGEOMETRY_HPP_
#define FENCEGEOMETRY_HPP_

#include <vector>
#include "common/core/Box.hpp"

namespace retro {

inline std::vector<FloatBox> fenceBoxes(bool west, bool east, bool north, bool south) {
	std::vector<FloatBox> boxes;
	boxes.reserve(5);
	boxes.emplace_back(6.f / 16, 6.f / 16, 0, 4.f / 16, 4.f / 16, 1);
	if (west || east) {
		float start = west ? 0 : 7.f / 16, end = east ? 1 : 9.f / 16;
		for (float height : {6.f / 16, 12.f / 16})
			boxes.emplace_back(start, 7.f / 16, height, end - start, 2.f / 16, 3.f / 16);
	}
	if (north || south) {
		float start = north ? 0 : 7.f / 16, end = south ? 1 : 9.f / 16;
		for (float height : {6.f / 16, 12.f / 16})
			boxes.emplace_back(7.f / 16, start, height, 2.f / 16, end - start, 3.f / 16);
	}
	return boxes;
}

}
#endif
