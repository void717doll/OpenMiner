#ifndef TORCHSUPPORT_HPP_
#define TORCHSUPPORT_HPP_

#include "common/world/World.hpp"

inline Vector3i torchSupportOffset(u16 rotation) {
	static const Vector3i offsets[] = {{0,0,-1}, {-1,0,0}, {0,-1,0}, {1,0,0}, {0,1,0}};
	return rotation < 5 ? offsets[rotation] : Vector3i{};
}

inline int torchRotationForFace(int face) {
	static constexpr int rotations[] = {1, 2, 0, 3, 4, -1};
	return face >= 0 && face < 6 ? rotations[face] : -1;
}

inline bool canPlaceTorch(const World &world, const Vector3i &pos, u16 rotation) {
	if (rotation > 4) return false;
	const auto anchor = pos + torchSupportOffset(rotation);
	const Chunk *chunk = world.getChunkAtBlockPos(anchor.x, anchor.y, anchor.z);
	if (!chunk || !chunk->isInitialized()) return false;
	const auto *state = world.getBlockState(anchor.x, anchor.y, anchor.z);
	if (!state || !state->isCollidable() || state->block().getGroupValue("group:om_piston")) return false;
	const auto &box = state->boundingBox();
	// mineclonia permits glass, ice, soul sand and spawners, but not lower slabs.
	if (state->block().stringID() == "default:soul_sand" || state->block().stringID() == "default:mob_spawner") return true;
	if (!rotation) {
		for (const char *group : {"group:fence", "group:wall", "group:pane", "group:anvil"})
			if (state->block().hasGroup(group)) return true;
		if (box.x == 0 && box.y == 0 && box.sizeX == 1 && box.sizeY == 1 && box.z + box.sizeZ == 1)
			return true;
	}
	return box.x == 0 && box.y == 0 && box.z == 0 && box.sizeX == 1 && box.sizeY == 1 && box.sizeZ == 1;
}

inline FloatBox torchSelectionBox(u16 rotation) {
	if (!rotation || rotation > 4) return {0.375f, 0.375f, 0, 0.25f, 0.25f, 0.625f};
	switch (rotation) {
		case 1: return {0, 0.375f, 0.2f, 0.65f, 0.25f, 0.625f};
		case 2: return {0.375f, 0, 0.2f, 0.25f, 0.65f, 0.625f};
		case 3: return {0.35f, 0.375f, 0.2f, 0.65f, 0.25f, 0.625f};
		default: return {0.375f, 0.35f, 0.2f, 0.25f, 0.65f, 0.625f};
	}
}

inline bool canPlaceCaveDecoration(const World &world, const Vector3i &pos) {
	const auto *floor = world.getBlockState(pos.x, pos.y, pos.z - 1);
	if (!floor || !floor->isCollidable() || floor->boundingBox().z + floor->boundingBox().sizeZ < 1) return false;
	for (int dz = -1; dz <= 1; ++dz) for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
		if (!dx && !dy && !dz) continue;
		const auto *neighbor = world.getBlockState(pos.x + dx, pos.y + dy, pos.z + dz);
		if (!neighbor || neighbor->block().hasGroup("group:om_cave_decoration")) return false;
	}
	return true;
}

#endif
