#ifndef WORLDBOUNDS_HPP_
#define WORLDBOUNDS_HPP_

namespace worldBounds {
constexpr int minHeight = 0;
constexpr int height = 256;
inline bool contains(int z) { return z >= minHeight && z < height; }
}

#endif
