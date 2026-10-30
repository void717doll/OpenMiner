#include <algorithm>
#include <cmath>

#include "common/core/Registry.hpp"
#include "common/world/Dimension.hpp"
#include "server/world/ServerWorld.hpp"

namespace {

bool loaded(const ServerWorld &world, const Vector3i &position) {
	const Chunk *chunk = world.getChunkAtBlockPos(position.x, position.y, position.z);
	return chunk && chunk->isInitialized();
}

bool frame(const ServerWorld &world, Vector3i origin, Vector3i axis, bool lit) {
	auto &definitions = Registry::getInstance();
	if (!definitions.hasBlock("default:obsidian") || !definitions.hasBlock("default:portal")) return false;
	u16 obsidian = definitions.getBlockFromStringID("default:obsidian").id();
	u16 portal = definitions.getBlockFromStringID("default:portal").id();
	for (int h = -1; h <= 2; ++h) {
		for (int z = -1; z <= 3; ++z) {
			bool side = h == -1 || h == 2, end = z == -1 || z == 3;
			if (side && end) continue;
			Vector3i point = origin + axis * h + Vector3i{0, 0, z};
			if (!loaded(world, point)) return false;
			u16 block = world.getBlock(point.x, point.y, point.z);
			if (side || end) { if (block != obsidian) return false; }
			else if (lit ? block != portal : block != 0 && block != portal) return false;
		}
	}
	return true;
}

void fillPortal(ServerWorld &world, Vector3i origin, Vector3i axis, bool buildFrame) {
	auto &definitions = Registry::getInstance();
	u16 obsidian = definitions.getBlockFromStringID("default:obsidian").id();
	const Block &portal = definitions.getBlockFromStringID("default:portal");
	for (int h = -1; h <= 2; ++h) {
		for (int z = -1; z <= 3; ++z) {
			Vector3i point = origin + axis * h + Vector3i{0, 0, z};
			bool edge = h == -1 || h == 2 || z == -1 || z == 3;
			if (edge && !buildFrame) continue;
			world.setBlock(point.x, point.y, point.z, edge ? obsidian : portal.id());
			if (!edge) world.setData(point.x, point.y, point.z,
				portal.param().setParam(BlockParam::State, 0, axis.y ? 1 : 0));
		}
	}
}

bool emptyRoom(const ServerWorld &world, Vector3i origin, bool requireGround) {
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 2; ++x) {
			for (int z = -2; z <= 3; ++z) {
				Vector3i point = origin + Vector3i{x, y, z};
				if (!loaded(world, point)) return false;
				const BlockState *state = world.getBlockState(point.x, point.y, point.z);
				if (!state || state->drawType() == BlockDrawType::Liquid) return false;
				if (z == -2 && requireGround) {
					if (!state->isCollidable() || state->boundingBox().sizeZ < 1) return false;
				}
				else if (state->block().id()) return false;
			}
		}
	}
	return true;
}

}

bool ServerWorld::portalFrameAt(Vector3i point, Vector3i &origin, Vector3i &axis) const {
	for (Vector3i direction : {Vector3i{1, 0, 0}, Vector3i{0, 1, 0}}) {
		for (int h = 0; h < 2; ++h) {
			for (int z = 0; z < 3; ++z) {
				Vector3i candidate = point - direction * h - Vector3i{0, 0, z};
				if (frame(*this, candidate, direction, true)) {
					origin = candidate; axis = direction; return true;
				}
			}
		}
	}
	return false;
}

bool ServerWorld::ignitePortal(const Vector3i &clicked) {
	if (m_dimension.id() > 1) return false;
	for (Vector3i axis : {Vector3i{1, 0, 0}, Vector3i{0, 1, 0}}) {
		for (int h = -1; h <= 2; ++h) {
			for (int z = -1; z <= 3; ++z) {
				Vector3i origin = clicked - axis * h - Vector3i{0, 0, z};
				if (!frame(*this, origin, axis, false) || frame(*this, origin, axis, true)) continue;
				fillPortal(*this, origin, axis, false);
				return true;
			}
		}
	}
	return false;
}

void ServerWorld::validatePortal(int x, int y, int z) {
	Vector3i point{x, y, z}, origin, axis;
	if (portalFrameAt(point, origin, axis)) return;
	// a missing neighboring chunk is not a broken frame.
	for (int dx = -2; dx <= 2; ++dx)
		for (int dy = -2; dy <= 2; ++dy)
			for (int dz = -3; dz <= 3; ++dz)
				if (!loaded(*this, point + Vector3i{dx, dy, dz})) return;
	const BlockState *state = getBlockState(x, y, z);
	if (state && state->block().stringID() == "default:portal") setBlock(x, y, z, 0);
}

bool ServerWorld::findPortalDestination(Vector3i requested, Vector3i &arrival) {
	if (m_dimension.id() > 1 || !Registry::getInstance().hasBlock("default:portal")) return false;
	Vector3i origin, axis;
	// a saved link can lead to a player-built portal outside the terrain height range.
	if (portalFrameAt(requested, origin, axis)) { arrival = origin; return true; }
	requested.z = std::clamp(requested.z, 35, 120);
	int centerX = int(std::floor(requested.x / 16.0)), centerY = int(std::floor(requested.y / 16.0));
	for (int cy = centerY - 1; cy <= centerY + 1; ++cy)
		for (int cx = centerX - 1; cx <= centerX + 1; ++cx)
			for (int cz = 0; cz < 4; ++cz) generateChunk(getOrCreateChunk(cx, cy, cz));

	u16 portal = Registry::getInstance().getBlockFromStringID("default:portal").id();
	double nearest = 1e30;
	for (int y = requested.y - 8; y <= requested.y + 8; ++y) {
		for (int x = requested.x - 8; x <= requested.x + 8; ++x) {
			for (int z = 2; z < 124; ++z) {
				if (getBlock(x, y, z) != portal || !portalFrameAt({x, y, z}, origin, axis)) continue;
				double distance = (origin - requested).length();
				if (distance < nearest) { arrival = origin; nearest = distance; }
			}
		}
	}
	if (nearest < 1e30) return true;

	// prefer an open room on solid ground; an empty cavern can receive a small platform.
	for (bool grounded : {true, false}) {
		for (int offset = 0; offset < 124; ++offset) {
			for (int direction : {1, -1}) {
				int z = requested.z + offset * direction;
				if (z < 3 || z > 123 || (!offset && direction == -1)) continue;
				for (int radius = 0; radius <= 8; ++radius) {
					for (int dy = -radius; dy <= radius; ++dy) {
						for (int dx = -radius; dx <= radius; ++dx) {
							if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
							Vector3i candidate{requested.x + dx, requested.y + dy, z};
							if (!emptyRoom(*this, candidate, grounded)) continue;
							u16 obsidian = Registry::getInstance().getBlockFromStringID("default:obsidian").id();
							for (int py = -1; py <= 1; ++py)
								for (int px = -1; px <= 2; ++px)
									setBlock(candidate.x + px, candidate.y + py, z - 1, obsidian);
							fillPortal(*this, candidate, {1, 0, 0}, true);
							arrival = candidate;
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}
