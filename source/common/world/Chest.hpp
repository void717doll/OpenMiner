#ifndef CHEST_HPP_
#define CHEST_HPP_

#include "common/world/World.hpp"
#include "common/world/WorldBounds.hpp"

namespace chest {

inline constexpr Vector3i neighbors[] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}};

inline const BlockState *loaded(const World &world, const Vector3i &pos) {
	const auto *chunk = world.getChunkAtBlockPos(pos.x, pos.y, pos.z);
	return chunk && chunk->isInitialized() ? world.getBlockState(pos.x, pos.y, pos.z) : nullptr;
}

inline bool canPlace(const World &world, const Vector3i &pos) {
	unsigned count = 0;
	for (const auto &offset : neighbors) {
		Vector3i next = pos + offset;
		const auto *state = loaded(world, next);
		if (!state) return false;
		if (!state->block().hasGroup("group:om_chest")) continue;
		if (++count > 1) return false;
		for (const auto &other : neighbors) {
			Vector3i adjacent = next + other;
			if (adjacent == pos) continue;
			const auto *neighbor = loaded(world, adjacent);
			if (!neighbor || neighbor->block().hasGroup("group:om_chest")) return false;
		}
	}
	return true;
}

inline std::vector<Vector3i> halves(const World &world, const Vector3i &pos) {
	const auto *state = loaded(world, pos);
	if (!state || !state->block().hasGroup("group:om_chest")) return {};
	std::vector<Vector3i> result{pos};
	// keep old, invalid clusters accessible as individual chests.
	if (!canPlace(world, pos)) return result;
	for (const auto &offset : neighbors) {
		const auto *next = loaded(world, pos + offset);
		if (next && next->block().id() == state->block().id()) {
			if (offset.x < 0 || offset.y < 0) result.insert(result.begin(), pos + offset);
			else result.push_back(pos + offset);
			break;
		}
	}
	return result;
}

inline bool canOpen(const World &world, const std::vector<Vector3i> &positions) {
	if (positions.empty()) return false;
	for (const auto &pos : positions) {
		if (!worldBounds::contains(pos.z + 1)) continue;
		const auto *above = loaded(world, pos + Vector3i{0, 0, 1});
		if (!above || above->isOpaque()) return false;
	}
	return true;
}

}

#endif
