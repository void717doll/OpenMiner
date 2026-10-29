#ifndef WORLDCOLLISION_HPP_
#define WORLDCOLLISION_HPP_

#include <cmath>

#include "common/core/BlockGeometry.hpp"
#include "common/world/World.hpp"

namespace collision {

inline DoubleBox blockBox(const BlockState &state, u16 data, int x, int y, int z) {
	DoubleBox box = state.boundingBox();
	if (state.block().isRotatable()) {
		BlockGeometry::initOrientation();
		u16 orientation = state.block().param().getParam(BlockParam::Rotation, data);
		const glm::mat3 &rotation = BlockGeometry::orientMatrices[orientation & 31];
		glm::vec3 a = rotation * glm::vec3(box.x - 0.5, box.y - 0.5, box.z - 0.5);
		glm::vec3 b = rotation * glm::vec3(box.x + box.sizeX - 0.5,
			box.y + box.sizeY - 0.5, box.z + box.sizeZ - 0.5);
		box.reset(std::min(a.x, b.x) + 0.5, std::min(a.y, b.y) + 0.5,
			std::min(a.z, b.z) + 0.5, std::abs(a.x - b.x), std::abs(a.y - b.y), std::abs(a.z - b.z));
	}
	box.move(x, y, z);
	return box;
}

inline double clipAxis(const DoubleBox &box, const DoubleBox &obstacle, int axis, double movement) {
	double low[3] = {box.x, box.y, box.z};
	double high[3] = {box.x + box.sizeX, box.y + box.sizeY, box.z + box.sizeZ};
	double otherLow[3] = {obstacle.x, obstacle.y, obstacle.z};
	double otherHigh[3] = {obstacle.x + obstacle.sizeX, obstacle.y + obstacle.sizeY, obstacle.z + obstacle.sizeZ};
	for (int i = 0; i < 3; ++i) {
		if (i != axis && (high[i] <= otherLow[i] || low[i] >= otherHigh[i]))
			return movement;
	}
	if (movement > 0 && high[axis] <= otherLow[axis])
		return std::min(movement, otherLow[axis] - high[axis]);
	if (movement < 0 && low[axis] >= otherHigh[axis])
		return std::max(movement, otherHigh[axis] - low[axis]);
	return movement;
}

inline Vector3d move(const World &world, DoubleBox box, Vector3d movement) {
	// resolve vertical motion first, then slide along each horizontal axis.
	for (int axis : {2, 0, 1}) {
		double amount = axis == 0 ? movement.x : axis == 1 ? movement.y : movement.z;
		double low[3] = {box.x, box.y, box.z};
		double high[3] = {box.x + box.sizeX, box.y + box.sizeY, box.z + box.sizeZ};
		low[axis] += std::min(0.0, amount);
		high[axis] += std::max(0.0, amount);
		for (int z = int(std::floor(low[2])); z <= int(std::floor(high[2])); ++z) {
			for (int y = int(std::floor(low[1])); y <= int(std::floor(high[1])); ++y) {
				for (int x = int(std::floor(low[0])); x <= int(std::floor(high[0])); ++x) {
					const BlockState *state = world.getBlockState(x, y, z);
					if (state && (!state->block().id() || !state->isCollidable()))
						continue;
					DoubleBox obstacle = state ? blockBox(*state, world.getData(x, y, z), x, y, z)
						: DoubleBox{double(x), double(y), double(z), 1, 1, 1};
					amount = clipAxis(box, obstacle, axis, amount);
				}
			}
		}
		if (axis == 0) { movement.x = amount; box.x += amount; }
		if (axis == 1) { movement.y = amount; box.y += amount; }
		if (axis == 2) { movement.z = amount; box.z += amount; }
	}
	return movement;
}

}

#endif // WORLDCOLLISION_HPP_
