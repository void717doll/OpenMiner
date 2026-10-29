#include <cmath>

#include <glm/gtc/quaternion.hpp>

#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/NetworkComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/world/Dimension.hpp"
#include "common/world/WorldCollision.hpp"
#include "server/core/PlayerList.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerWorld.hpp"

namespace {

u32 nextRandom(MobComponent &mob) {
	mob.randomState = mob.randomState * 1664525u + 1013904223u;
	return mob.randomState;
}

bool clearPath(const World &world, Vector3d from, Vector3d to) {
	Vector3d delta = to - from;
	int steps = std::max(1, int(std::ceil(delta.length() * 4)));
	for (int i = 1; i < steps; ++i) {
		Vector3d point = from + delta * (double(i) / steps);
		const BlockState *state = world.getBlockState(int(std::floor(point.x)), int(std::floor(point.y)), int(std::floor(point.z)));
		if (!state || (state->block().id() && state->isCollidable())) return false;
	}
	return true;
}

}

bool ServerWorld::spawnMob(const std::string &id, double x, double y, double z) {
	if (m_scene.registry().view<MobComponent>().size() >= 64) return false;
	Registry &definitions = Registry::getInstance();
	entt::entity model = definitions.getEntityFromStringID(id);
	if (model == entt::null || !definitions.entityRegistry().all_of<MobComponent, DoubleBox, RotationComponent>(model)) return false;
	const Chunk *chunk = getChunkAtBlockPos(int(std::floor(x)), int(std::floor(y)), int(std::floor(z)));
	if (!chunk || !chunk->isInitialized()) return false;
	const DoubleBox &localBox = definitions.entityRegistry().get<DoubleBox>(model);
	DoubleBox box = localBox;
	box.move(x, y, z);
	for (int bz = int(std::floor(box.z)); bz <= int(std::floor(box.z + box.sizeZ)); ++bz) {
		for (int by = int(std::floor(box.y)); by <= int(std::floor(box.y + box.sizeY)); ++by) {
			for (int bx = int(std::floor(box.x)); bx <= int(std::floor(box.x + box.sizeX)); ++bx) {
				const BlockState *state = getBlockState(bx, by, bz);
				if (!state || (state->block().id() && state->isCollidable()
				 && collision::blockBox(*state, getData(bx, by, bz), bx, by, bz).intersects(box))) return false;
			}
		}
	}
	entt::entity entity = m_scene.createEntityFromModel(definitions.entityRegistry(), model);
	auto &registry = m_scene.registry();
	registry.emplace<PositionComponent>(entity, x, y, z, m_dimension.id());
	registry.emplace<NetworkComponent>(entity, entity);
	registry.emplace<std::string>(entity, id);
	registry.get<MobComponent>(entity).randomState = u32(m_seed) ^ u32(entity) ^ 0x9e3779b9u;
	return true;
}

void ServerWorld::updateMobs() {
	auto &registry = m_scene.registry();
	std::vector<entt::entity> dead;
	registry.view<MobComponent, PositionComponent, DoubleBox, RotationComponent>().each(
		[&](auto entity, auto &mob, auto &position, auto &localBox, auto &rotation) {
			if (!mob.health) { dead.emplace_back(entity); return; }
			const Chunk *current = getChunkAtBlockPos(int(std::floor(position.x)), int(std::floor(position.y)), int(std::floor(position.z)));
			if (!current || !current->isInitialized()) return;
			++mob.age;
			if (mob.attackCooldown) --mob.attackCooldown;
			ServerPlayer *target = nullptr;
			double nearest = 16 * 16;
			for (auto &entry : m_players) {
				auto &player = entry.second;
				if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
				double dx = player.x() - position.x, dy = player.y() - position.y, dz = player.z() - position.z;
				double distance = dx * dx + dy * dy + dz * dz;
				if (mob.hostile && distance < nearest && clearPath(*this,
					{position.x, position.y, position.z + localBox.sizeZ * 0.8}, {player.x(), player.y(), player.z() + 1.5})) {
					target = &player;
					nearest = distance;
				}
			}
			if (target) {
				mob.yaw = float(std::atan2(target->y() - position.y, target->x() - position.x));
				if (nearest < 4 && mob.attackCooldown == 0 && m_server) {
					mob.attackCooldown = 20;
					m_server->damagePlayer(target->clientID(), mob.attackStrength);
				}
			}
			else if (mob.age % 60 == 1)
				mob.yaw = float(nextRandom(mob) % 6284) / 1000.f;

			bool walking = target || (mob.age % 100 < 60);
			double speed = walking ? mob.moveSpeed * 0.2 : 0;
			mob.verticalSpeed = std::max(-0.5f, (mob.verticalSpeed - 0.08f) * 0.98f);
			Vector3d desired{std::cos(mob.yaw) * speed, std::sin(mob.yaw) * speed, mob.verticalSpeed};
			DoubleBox box = localBox;
			box.move(position.x, position.y, position.z);
			Vector3d movement = collision::move(*this, box, desired);
			bool grounded = desired.z < 0 && movement.z > desired.z;
			if (movement.z != desired.z) mob.verticalSpeed = 0;
			if (grounded && (std::abs(movement.x - desired.x) > 0.001 || std::abs(movement.y - desired.y) > 0.001))
				mob.verticalSpeed = 0.42f;
			position.x += movement.x; position.y += movement.y; position.z += movement.z;
			position.isUpdated = position.isUpdated || !movement.isZero();
			rotation.quat = glm::angleAxis(mob.yaw + float(M_PI / 2), glm::vec3(0, 0, 1));
			rotation.isUpdated = true;

			if (mob.hostile && mob.age % 20 == 0 && GameTime::getTicks() % GameTime::dayLength < 12000
			 && position.z >= m_heightmap.getHighestBlockAt(int(std::floor(position.x)), int(std::floor(position.y))) + 1) {
				--mob.health;
				if (!mob.health) dead.emplace_back(entity);
			}
		});
	for (auto entity : dead) {
		if (m_server) m_server->sendEntityDespawn(entity);
		registry.destroy(entity);
	}

	if (++m_mobTick % 200 != 0 || m_dimension.id() != 0) return;
	bool night = GameTime::getTicks() % GameTime::dayLength >= 12000;
	for (const auto &entry : m_players) {
		const auto &player = entry.second;
		if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
		int x = int(std::floor(player.x())) + int((m_mobTick * 17u) % 25) - 12;
		int y = int(std::floor(player.y())) + int((m_mobTick * 29u) % 25) - 12;
		int z = m_heightmap.getHighestBlockAt(x, y) + 1;
		if (z <= SEALEVEL) continue;
		const BlockState *ground = getBlockState(x, y, z - 1);
		if (!ground || (!night && ground->block().stringID() != "default:grass")) continue;
		spawnMob(night ? "default:zombie" : "default:pig", x + 0.5, y + 0.5, z);
	}
}

bool ServerWorld::attackMob(ServerPlayer &player) {
	Vector3d origin{player.x(), player.y(), player.z() + 1.625};
	double yaw = player.cameraYaw() * math::DEG_TO_RAD, pitch = player.cameraPitch() * math::DEG_TO_RAD;
	Vector3d direction{std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch), std::sin(pitch)};
	entt::entity selected = entt::null;
	double nearest = 3;
	auto &registry = m_scene.registry();
	registry.view<MobComponent, PositionComponent, DoubleBox>().each([&](auto entity, auto &, auto &position, auto &hitbox) {
		DoubleBox box = hitbox;
		box.move(position.x, position.y, position.z);
		double low[3] = {box.x, box.y, box.z}, high[3] = {box.x + box.sizeX, box.y + box.sizeY, box.z + box.sizeZ};
		double start[3] = {origin.x, origin.y, origin.z}, ray[3] = {direction.x, direction.y, direction.z};
		double enter = 0, leave = nearest;
		for (int axis = 0; axis < 3; ++axis) {
			if (std::abs(ray[axis]) < 1e-9) {
				if (start[axis] < low[axis] || start[axis] > high[axis]) return;
				continue;
			}
			double a = (low[axis] - start[axis]) / ray[axis], b = (high[axis] - start[axis]) / ray[axis];
			if (a > b) std::swap(a, b);
			enter = std::max(enter, a); leave = std::min(leave, b);
			if (enter > leave) return;
		}
		if (enter < nearest && clearPath(*this, origin, origin + direction * enter)) {
			nearest = enter;
			selected = entity;
		}
	});
	if (selected == entt::null) return false;
	auto &mob = registry.get<MobComponent>(selected);
	if (mob.health) --mob.health;
	if (!mob.health) {
		u16 count = u16(nextRandom(mob) % 3);
		if (count && !mob.dropItem.empty()) player.inventory().addStack(mob.dropItem, count);
		if (m_server) {
			m_server->sendPlayerInvUpdate(player.clientID());
			m_server->sendEntityDespawn(selected);
		}
		registry.destroy(selected);
	}
	return true;
}
