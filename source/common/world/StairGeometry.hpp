#ifndef STAIRGEOMETRY_HPP_
#define STAIRGEOMETRY_HPP_

#include <array>
#include "common/core/Box.hpp"
#include "common/core/IntTypes.hpp"

namespace retro {

// beta stairs have two upright pieces and ascend in the player's facing direction.
inline std::array<FloatBox, 2> stairBoxes(u16 rotation) {
	switch (rotation & 3) {
		case 1: return {{{0, 0, 0, 1, 0.5f, 0.5f}, {0, 0.5f, 0, 1, 0.5f, 1}}};
		case 2: return {{{0.5f, 0, 0, 0.5f, 1, 0.5f}, {0, 0, 0, 0.5f, 1, 1}}};
		case 3: return {{{0, 0.5f, 0, 1, 0.5f, 0.5f}, {0, 0, 0, 1, 0.5f, 1}}};
		default: return {{{0, 0, 0, 0.5f, 1, 0.5f}, {0.5f, 0, 0, 0.5f, 1, 1}}};
	}
}

}
#endif
