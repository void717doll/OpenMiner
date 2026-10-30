#ifndef SIMPLETREESHAPE_HPP_
#define SIMPLETREESHAPE_HPP_

namespace retro {

inline unsigned simpleTreeBlock(int x, int y, int z, int height) {
	if (!x && !y && z >= 0 && z < height) return 1;
	int dz = z - height;
	return z >= 0 && x * x + y * y + dz * dz < 9 ? 2 : 0;
}

}

#endif // SIMPLETREESHAPE_HPP_
