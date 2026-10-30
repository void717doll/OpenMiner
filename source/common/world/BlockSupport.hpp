#ifndef BLOCKSUPPORT_HPP_
#define BLOCKSUPPORT_HPP_

#include "common/world/TorchSupport.hpp"
#include "common/world/WorldCollision.hpp"

inline const BlockState *loadedSupport(const World &world, const Vector3i &pos) {
	const Chunk *chunk = world.getChunkAtBlockPos(pos.x, pos.y, pos.z);
	return chunk && chunk->isInitialized() ? world.getBlockState(pos.x, pos.y, pos.z) : nullptr;
}

inline bool hasSolidTop(const World &world, const Vector3i &pos) {
	const auto *state = loadedSupport(world, pos);
	if (!state || !state->block().id() || !state->isCollidable()) return false;
	const auto box = collision::blockBox(*state, world.getData(pos.x, pos.y, pos.z), 0, 0, 0);
	// modernmcp permits full opaque blocks and the full top face of upper slabs.
	return box.x == 0 && box.y == 0 && box.sizeX == 1 && box.sizeY == 1 && box.z + box.sizeZ == 1
		&& (state->isOpaque() || box.z > 0);
}

inline bool blockHasSupport(const World &world, const Vector3i &pos, const BlockState &state, u16 data) {
	const auto &block = state.block();
	if (!loadedSupport(world, pos) || !block.placementConstraints().check(world, pos)) return false;
	if (block.hasGroup("group:om_cave_decoration") && !canPlaceCaveDecoration(world, pos)) return false;
	u16 rotation = block.isRotatable() ? block.param().getParam(BlockParam::Rotation, data) : 0;
	u16 attached = block.getGroupValue("group:attached_node");
	if (attached == 5 || block.hasGroup("group:om_torch") || block.hasGroup("group:om_wall_switch"))
		return canPlaceTorch(world, pos, rotation) && (block.getGroupValue("group:om_wall_switch") != 2 || rotation != 0);
	if (block.hasGroup("group:om_rail")) {
		if (!hasSolidTop(world, pos + Vector3i{0, 0, -1})) return false;
		unsigned shape = block.getGroupValue("group:om_rail_type") ? state.id() & 7 : state.id();
		if (shape >= 2 && shape <= 5) {
			static constexpr Vector3i highEnds[] = {{1,0,0}, {-1,0,0}, {0,-1,0}, {0,1,0}};
			return hasSolidTop(world, pos + highEnds[shape - 2]);
		}
		return true;
	}
	u16 source = block.getGroupValue("group:om_power_source");
	if (source == 6 || source == 7) {
		Vector3i below = pos + Vector3i{0, 0, -1};
		const auto *floor = loadedSupport(world, below);
		return hasSolidTop(world, below) || (source == 7 && floor && floor->block().stringID() == "default:glowstone");
	}
	if (!attached) return true;
	Vector3i offset{0, 0, attached == 4 ? 1 : -1};
	if ((attached == 1 || attached == 2) && block.isRotatable()) {
		BlockGeometry::initOrientation();
		glm::vec3 direction = BlockGeometry::orientMatrices[rotation & 31] * glm::vec3(0, 0, -1);
		offset = {int(std::round(direction.x)), int(std::round(direction.y)), int(std::round(direction.z))};
	}
	const auto *support = loadedSupport(world, pos + offset);
	return support && (support->isCollidable() || (attached == 4
		&& block.hasGroup("group:om_hanging_chain") && support->block().hasGroup("group:om_hanging_chain")));
}

#endif
