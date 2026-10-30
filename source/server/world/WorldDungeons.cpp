#include "common/core/Registry.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/world/Dimension.hpp"
#include "common/world/RetroRandom.hpp"
#include "common/world/WorldCollision.hpp"
#include "server/core/PlayerList.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerWorld.hpp"

void ServerWorld::tickSpawner(int x, int y, int z) {
	auto *chunk = static_cast<ServerChunk *>(getChunkAtBlockPos(x, y, z));
	const auto *state = getBlockState(x, y, z);
	if (!chunk || !chunk->isInitialized() || !state || state->block().stringID() != "default:mob_spawner") return;
	bool active = false;
	for (const auto &entry : m_players) {
		const auto &player = entry.second;
		if (!player.isOnline() || !player.isReady() || !player.health() || player.dimension() != m_dimension.id()) continue;
		double dx = player.x() - x - 0.5, dy = player.y() - y - 0.5, dz = player.z() - z - 0.5;
		if (dx * dx + dy * dy + dz * dz < 16 * 16) { active = true; break; }
	}
	if (!active) return;
	auto *data = addBlockData(x, y, z);
	auto &meta = data->meta;
	int delay = meta.hasInt("spawner_delay") ? meta.get<int>("spawner_delay") : 20;
	if (m_server && delay % 20 == 0) m_server->sendParticles({x + 0.5, y + 0.5, z + 0.5}, m_dimension.id(), 1);
	if (delay > 0) {
		meta.setInt("spawner_delay", delay - 1); chunk->setModified(true); return;
	}
	u32 seed = meta.hasInt("spawner_random") ? u32(meta.get<int>("spawner_random")) : u32(m_seed) ^ u32(x) ^ u32(y) * 31u ^ u32(z) * 131u;
	seed = (seed * 1103515245u + 12345u) & 0x7fffffffu;
	meta.setInt("spawner_random", int(seed)); chunk->setModified(true);
	retro::Random random(seed);
	auto resetDelay = [&]() { meta.setInt("spawner_delay", 200 + random.nextInt(600)); };
	if (delay < 0) { resetDelay(); return; }
	int kind = meta.hasInt("spawner_kind") ? meta.get<int>("spawner_kind") : 1;
	const char *id = kind == 0 ? "default:skeleton" : kind == 3 ? "default:spider" : "default:zombie";
	auto &definitions = Registry::getInstance();
	auto model = definitions.getEntityFromStringID(id);
	if (model == entt::null || !definitions.entityRegistry().all_of<DoubleBox>(model)) return;
	DoubleBox nearby{double(x - 8), double(y - 8), double(z - 4), 17, 17, 9};
	for (int attempt = 0; attempt < 4; ++attempt) {
		unsigned count = 0;
		m_scene.registry().view<MobComponent, BetaMobComponent, PositionComponent, DoubleBox>().each(
			[&](auto, const auto &mob, const auto &traits, const auto &position, const auto &hitbox) {
				if (!mob.health || traits.kind != betaMobKind(id) || position.dimension != m_dimension.id()) return;
				DoubleBox box = hitbox; box.move(position.x, position.y, position.z);
				if (box.intersects(nearby)) ++count;
			});
		if (count >= 6) { resetDelay(); return; }
		double spawnX = x + (random.nextDouble() - random.nextDouble()) * 4;
		double spawnZ = z + random.nextInt(3) - 1;
		double spawnY = y + (random.nextDouble() - random.nextDouble()) * 4;
		random.nextFloat();
		int bx = int(std::floor(spawnX)), by = int(std::floor(spawnY)), bz = int(std::floor(spawnZ));
		const auto *lightChunk = getChunkAtBlockPos(bx, by, bz);
		if (!lightChunk || !lightChunk->isInitialized()) continue;
		int sky = lightChunk->lightmap().getSunlight(bx & 15, by & 15, bz & (CHUNK_HEIGHT - 1));
		if (sky > random.nextInt(32) || getLight(bx, by, bz) > random.nextInt(8)) continue;
		DoubleBox box = definitions.entityRegistry().get<DoubleBox>(model); box.move(spawnX, spawnY, spawnZ);
		bool clear = true;
		for (int pz = int(std::floor(box.z)); clear && pz < int(std::ceil(box.z + box.sizeZ)); ++pz)
			for (int py = int(std::floor(box.y)); clear && py < int(std::ceil(box.y + box.sizeY)); ++py)
				for (int px = int(std::floor(box.x)); clear && px < int(std::ceil(box.x + box.sizeX)); ++px) {
					const auto *candidate = getChunkAtBlockPos(px, py, pz);
					const auto *block = getBlockState(px, py, pz);
					if (!candidate || !candidate->isInitialized() || !block || block->drawType() == BlockDrawType::Liquid) clear = false;
					else if (block->isCollidable() && collision::blockBox(*block, getData(px, py, pz), px, py, pz).intersects(box)) clear = false;
				}
		std::vector<DoubleBox> occupied;
		appendCollisionBoxes(box, box, occupied);
		if (!occupied.empty()) clear = false;
		if (clear && spawnMob(id, spawnX, spawnY, spawnZ)) {
			resetDelay();
			if (m_server) m_server->sendParticles({x + 0.5, y + 0.5, z + 0.5}, m_dimension.id(), 2);
		}
	}
}
