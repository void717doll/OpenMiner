#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/world/Dimension.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerWorld.hpp"

bool ServerWorld::tryCreateSnowGolem(const Vector3i &pumpkin) {
	u16 ids[3], data[3];
	for (int offset = 0; offset < 3; ++offset) {
		int z = pumpkin.z - offset;
		const Chunk *chunk = getChunkAtBlockPos(pumpkin.x, pumpkin.y, z);
		const BlockState *state = getBlockState(pumpkin.x, pumpkin.y, z);
		if (!chunk || !chunk->isInitialized() || !state || getBlockData(pumpkin.x, pumpkin.y, z)) return false;
		if (state->block().stringID() != (offset ? "default:snow" : "default:pumpkin")) return false;
		ids[offset] = state->block().id(); data[offset] = getData(pumpkin.x, pumpkin.y, z);
	}
	// spawning checks the final collision box; restore the tower if the mob cap or model prevents creation.
	for (int offset = 0; offset < 3; ++offset) setBlock(pumpkin.x, pumpkin.y, pumpkin.z - offset, 0);
	if (!spawnMob("default:snow_golem", pumpkin.x + 0.5, pumpkin.y + 0.5, pumpkin.z - 2)) {
		for (int offset = 2; offset >= 0; --offset) {
			setBlock(pumpkin.x, pumpkin.y, pumpkin.z - offset, ids[offset]);
			setData(pumpkin.x, pumpkin.y, pumpkin.z - offset, data[offset]);
		}
		return false;
	}
	if (m_server) m_server->sendParticles({pumpkin.x + 0.5, pumpkin.y + 0.5, pumpkin.z - 1.0}, m_dimension.id(), 2);
	return true;
}
