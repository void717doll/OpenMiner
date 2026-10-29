#ifndef HEARTBARSTATE_HPP_
#define HEARTBARSTATE_HPP_

#include <algorithm>

namespace hearts {

constexpr unsigned count = 10;
constexpr float size = 9.f;
constexpr float spacing = 8.f;
constexpr float hotbarOffset = 10.f;

enum class Fill : unsigned char { Empty, Half, Full };

inline Fill fill(unsigned health, unsigned slot) {
	if (slot >= count) return Fill::Empty;
	health = std::min(health, count * 2);
	unsigned threshold = slot * 2;
	if (health <= threshold) return Fill::Empty;
	return health == threshold + 1 ? Fill::Half : Fill::Full;
}

}

#endif // HEARTBARSTATE_HPP_
