#ifndef RETROMOBSPAWN_HPP_
#define RETROMOBSPAWN_HPP_

#include "common/world/RetroRandom.hpp"
#include "common/world/WorldCollision.hpp"

namespace retro {

inline bool slimeChunk(std::int64_t seed, int x, int y) {
	// chunk.java casts after these int products, so overflow occurs before promotion.
	auto signedInt = [](std::uint32_t value) {
		return value < 0x80000000u ? std::int64_t(value) : std::int64_t(value) - 0x100000000LL;
	};
	std::uint32_t cx = std::uint32_t(x), cy = std::uint32_t(y);
	std::uint64_t mixed = std::uint64_t(seed) + std::uint64_t(signedInt(cx * cx * 4987142u))
		+ std::uint64_t(signedInt(cx * 5947611u)) + std::uint64_t(signedInt(cy * cy)) * 4392871ULL
		+ std::uint64_t(signedInt(cy * 389711u));
	Random random(std::int64_t(mixed ^ 987234911ULL));
	return random.nextInt(10) == 0;
}

inline int mobCavityFloor(const World &world, DoubleBox hitbox, double x, double y, int preferredZ,
		int distance = 16, int maximumZ = 126) {
	preferredZ = std::max(1, std::min(maximumZ, preferredZ));
	for (int offset = 0; offset <= distance; ++offset) {
		for (int direction : {-1, 1}) {
			if (!offset && direction == 1) continue;
			int z = preferredZ + offset * direction;
			if (z < 1 || z > maximumZ) continue;
			int groundX = int(std::floor(x)), groundY = int(std::floor(y));
			const Chunk *groundChunk = world.getChunkAtBlockPos(groundX, groundY, z - 1);
			const BlockState *ground = world.getBlockState(groundX, groundY, z - 1);
			if (!groundChunk || !groundChunk->isInitialized() || !ground || !ground->block().id()
			 || !ground->isCollidable() || ground->drawType() == BlockDrawType::Liquid) continue;
			DoubleBox support = collision::blockBox(*ground, world.getData(groundX, groundY, z - 1), groundX, groundY, z - 1);
			if (std::abs(support.z + support.sizeZ - z) > 0.001) continue;
			DoubleBox box = hitbox;
			box.move(x, y, z);
			bool clear = true;
			for (int bz = int(std::floor(box.z)); clear && bz < int(std::ceil(box.z + box.sizeZ)); ++bz)
				for (int by = int(std::floor(box.y)); clear && by < int(std::ceil(box.y + box.sizeY)); ++by)
					for (int bx = int(std::floor(box.x)); clear && bx < int(std::ceil(box.x + box.sizeX)); ++bx) {
						const Chunk *chunk = world.getChunkAtBlockPos(bx, by, bz);
						const BlockState *state = world.getBlockState(bx, by, bz);
						if (!chunk || !chunk->isInitialized() || !state || state->drawType() == BlockDrawType::Liquid)
							clear = false;
						else if (state->block().id() && state->isCollidable()
						 && collision::blockBox(*state, world.getData(bx, by, bz), bx, by, bz).intersects(box)) clear = false;
					}
			if (clear && box.z + box.sizeZ < 128) return z;
		}
	}
	return -1;
}

}

#endif // RETROMOBSPAWN_HPP_
