#include <cmath>
#include <unordered_set>

#include <glm/gtc/quaternion.hpp>

#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/NetworkComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"
#include "common/world/Dimension.hpp"
#include "common/world/WorldCollision.hpp"
#include "common/world/BetaMining.hpp"
#include "common/world/BetaToolRules.hpp"
#include "common/world/LiquidState.hpp"
#include "common/world/RetroMobSpawn.hpp"
#include "server/core/PlayerList.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerWorld.hpp"

namespace {

using Kind = BetaMobKind;

u32 nextRandom(MobComponent &mob) {
	mob.randomState = mob.randomState * 1664525u + 1013904223u;
	return mob.randomState;
}

double unitRandom(MobComponent &mob) { return double(nextRandom(mob)) / 4294967296.0; }

bool projectile(Kind kind) { return kind == Kind::Arrow || kind == Kind::Fireball; }

bool liquidAt(const World &world, Vector3d point) {
	int x = int(std::floor(point.x)), y = int(std::floor(point.y)), z = int(std::floor(point.z));
	const BlockState *state = world.getBlockState(x, y, z);
	if (!state || state->drawType() != BlockDrawType::Liquid) return false;
	const BlockState *above = world.getBlockState(x, y, z + 1);
	bool covered = above && liquid::kind(above->block()) == liquid::kind(state->block());
	return point.z - z < liquid::height(state->block(), world.getData(x, y, z), covered);
}

bool segmentBox(Vector3d start, Vector3d delta, const DoubleBox &box, double &distance) {
	double hit = collision::rayDistance(start, delta, box, distance);
	if (hit < 0) return false;
	distance = hit;
	return true;
}

bool clearPath(const World &world, Vector3d from, Vector3d to) {
	Vector3d delta = to - from;
	int steps = std::max(1, int(std::ceil(delta.length() * 8)));
	for (int i = 1; i < steps; ++i) {
		Vector3d point = from + delta * (double(i) / steps);
		int x = int(std::floor(point.x)), y = int(std::floor(point.y)), z = int(std::floor(point.z));
		const BlockState *state = world.getBlockState(x, y, z);
		if (!state) return false;
		if (!state->block().id() || !state->isCollidable()) continue;
		DoubleBox box = collision::blockBox(*state, world.getData(x, y, z), x, y, z);
		if (point.x >= box.x && point.x <= box.x + box.sizeX && point.y >= box.y && point.y <= box.y + box.sizeY
		 && point.z >= box.z && point.z <= box.z + box.sizeZ) return false;
	}
	return true;
}

BetaMobComponent &traitsFor(entt::registry &registry, entt::entity entity) {
	auto &traits = registry.get_or_emplace<BetaMobComponent>(entity);
	if (traits.kind == Kind::Unknown) {
		const auto *id = registry.try_get<std::string>(entity);
		if (id) traits.kind = betaMobKind(*id);
		if (traits.kind == Kind::Slime) traits.size = 2;
	}
	return traits;
}

void setSlimeSize(entt::registry &registry, entt::entity entity, u8 size, bool resetHealth) {
	auto &mob = registry.get<MobComponent>(entity);
	auto &traits = traitsFor(registry, entity);
	traits.size = size;
	registry.get<DoubleBox>(entity).reset(-0.3 * size, -0.3 * size, 0, 0.6 * size, 0.6 * size, 0.6 * size);
	if (resetHealth) mob.health = u16(size * size);
	mob.attackStrength = size;
	if (auto *visual = registry.try_get<DrawableDef>(entity)) {
		visual->setMobModel(u8(Kind::Slime), float(size));
		visual->isUpdated = true;
	}
}

void syncAppearance(entt::registry &registry, entt::entity entity) {
	auto *visual = registry.try_get<DrawableDef>(entity);
	if (!visual) return;
	const auto &traits = registry.get<BetaMobComponent>(entity);
	u8 appearance = 0;
	if (traits.kind == Kind::Sheep) appearance = u8((traits.color & 15) | (traits.sheared ? 16 : 0));
	if (traits.kind == Kind::Wolf) {
		appearance = u8((traits.anger ? 1 : 0) | (traits.tamed ? 2 : 0) | (traits.sitting ? 4 : 0));
		appearance |= u8(std::min<u16>(20, registry.get<MobComponent>(entity).health) << 3);
	}
	if (traits.kind == Kind::Pig) appearance = traits.saddled ? 1 : 0;
	if (traits.kind == Kind::Ghast) appearance = traits.attackCharge > 10 ? 1 : 0;
	if (visual->mobAppearance() != appearance) {
		visual->setMobAppearance(appearance);
		visual->isUpdated = true;
	}
}

bool dropItem(ServerWorld &world, Vector3d position, const std::string &id, u16 count, u16 damage = 0) {
	if (!count || !Registry::getInstance().hasItem(id)) return false;
	auto &definitions = Registry::getInstance();
	entt::entity model = definitions.getEntityFromStringID("default:item_drop");
	if (model == entt::null) return false;
	auto &scene = world.scene();
	if (scene.registry().view<ItemStack>().size() >= 128) return false;
	entt::entity entity = scene.createEntityFromModel(definitions.entityRegistry(), model);
	auto &registry = scene.registry();
	registry.emplace<PositionComponent>(entity, position.x, position.y, position.z + 0.3, world.dimension().id());
	registry.emplace<NetworkComponent>(entity, entity);
	registry.emplace<std::string>(entity, "default:item_drop");
	registry.emplace<ItemStack>(entity, id, count, damage);
	if (auto *drawable = registry.try_get<DrawableDef>(entity)) drawable->getInventoryCubeDef().blockID = id;
	return true;
}

struct Shot { Vector3d position, direction; Kind kind; entt::entity shooter; };
struct Blast { Vector3d position; float strength; u32 seed; };

void spawnShot(ServerWorld &world, const Shot &shot) {
	auto &registry = world.scene().registry();
	std::size_t count = 0;
	registry.view<BetaMobComponent>().each([&](auto, auto &traits) { if (projectile(traits.kind)) ++count; });
	if (count >= 32) return;
	entt::entity entity = registry.create();
	auto &mob = registry.emplace<MobComponent>(entity);
	mob.health = 1; mob.moveSpeed = 0; mob.attackStrength = shot.kind == Kind::Arrow ? 4 : 0;
	auto &traits = registry.emplace<BetaMobComponent>(entity);
	traits.kind = shot.kind;
	traits.owner = "mob:" + std::to_string(u32(shot.shooter));
	Vector3d direction = shot.direction;
	double length = direction.length();
	if (length < 1e-8) direction = {1, 0, 0};
	else direction /= length;
	traits.velocity = shot.kind == Kind::Arrow ? Vector3f(direction * 0.6) : Vector3f{0, 0, 0};
	traits.waypoint = Vector3f(direction * 0.1);
	registry.emplace<PositionComponent>(entity, shot.position.x, shot.position.y, shot.position.z, world.dimension().id());
	registry.emplace<NetworkComponent>(entity, entity);
	registry.emplace<std::string>(entity, "default:" + std::string(betaMobName(shot.kind)));
	registry.emplace<DoubleBox>(entity, -0.125, -0.125, -0.125, 0.25, 0.25, 0.25);
	registry.emplace<RotationComponent>(entity);
	registry.emplace<DrawableDef>(entity).setMobModel(u8(shot.kind));
}

void explode(ServerWorld &world, const Blast &blast) {
	float reach = blast.strength * 2;
	for (const auto &entry : world.players()) {
		const auto &player = entry.second;
		if (!player.isReady() || !player.isOnline() || player.dimension() != world.dimension().id()) continue;
		Vector3d target{player.x(), player.y(), player.z() + 0.8};
		double distance = (target - blast.position).length() / reach;
		if (distance > 1 || !clearPath(world, blast.position, target)) continue;
		double impact = 1 - distance;
		u16 damage = u16((impact * impact + impact) * 4 * reach + 1);
		if (world.server()) world.server()->damagePlayer(player.clientID(), damage);
	}
	world.scene().registry().view<MobComponent, PositionComponent>().each([&](auto entity, auto &mob, auto &position) {
		if (projectile(traitsFor(world.scene().registry(), entity).kind)) return;
		Vector3d target{position.x, position.y, position.z + 0.5};
		double distance = (target - blast.position).length() / reach;
		if (distance > 1 || !clearPath(world, blast.position, target)) return;
		u16 damage = u16(((1 - distance) * (1 - distance) + 1 - distance) * 4 * reach + 1);
		mob.health = damage >= mob.health ? 0 : mob.health - damage;
	});
	// beta's edge-ray blast propagation, bounded to already loaded terrain.
	MobComponent random; random.randomState = blast.seed;
	std::unordered_set<Vector3i> blocks;
	for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) for (int z = 0; z < 16; ++z) {
		if (x && x != 15 && y && y != 15 && z && z != 15) continue;
		Vector3d direction{x / 15.0 * 2 - 1, y / 15.0 * 2 - 1, z / 15.0 * 2 - 1};
		direction /= direction.length();
		double energy = blast.strength * (0.7 + unitRandom(random) * 0.6);
		Vector3d point = blast.position;
		for (int step = 0; energy > 0 && step < 80; ++step) {
			int bx = int(std::floor(point.x)), by = int(std::floor(point.y)), bz = int(std::floor(point.z));
			const Chunk *chunk = world.getChunkAtBlockPos(bx, by, bz);
			if (!chunk || !chunk->isInitialized()) break;
			const BlockState *state = world.getBlockState(bx, by, bz);
			if (state && state->block().id()) {
				double resistance = std::max(0.f, state->hardness());
				const auto &id = state->block().stringID();
				if (id == "default:obsidian" || id == "default:bedrock" || state->hardness() < 0 || !std::isfinite(state->hardness())) resistance = 1200;
				else if (id == "default:stone" || id == "default:cobblestone") resistance = 6;
				else if (state->drawType() == BlockDrawType::Liquid) resistance = 500;
				energy -= (resistance + 0.3) * 0.3;
			}
			if (energy > 0) blocks.emplace(bx, by, bz);
			point += direction * 0.3;
			energy -= 0.225;
		}
	}
	for (const auto &point : blocks) {
		if (world.getBlock(point.x, point.y, point.z)) world.setBlock(point.x, point.y, point.z, 0);
	}
}

bool consumeFor(ServerPlayer &player, const std::string &replacement) {
	ItemStack held = player.heldItemStack();
	if (!held.amount()) return false;
	if (held.amount() == 1) player.inventory().setStack(player.heldItemSlot(), 0, replacement, replacement.empty() ? 0 : 1);
	else {
		if (!replacement.empty() && player.inventory().addStack(replacement, 1).amount()) return false;
		player.inventory().setStack(player.heldItemSlot(), 0, held.item().stringID(), held.amount() - 1);
	}
	return true;
}

}

bool ServerWorld::spawnMob(const std::string &id, double x, double y, double z, u8 slimeSize) {
	std::size_t shots = 0;
	m_scene.registry().view<BetaMobComponent>().each([&](auto, auto &traits) { if (projectile(traits.kind)) ++shots; });
	if (m_scene.registry().view<MobComponent>().size() >= 64 + shots) return false;
	Registry &definitions = Registry::getInstance();
	entt::entity model = definitions.getEntityFromStringID(id);
	if (model == entt::null || !definitions.entityRegistry().all_of<MobComponent, DoubleBox, RotationComponent>(model)) return false;
	const Chunk *chunk = getChunkAtBlockPos(int(std::floor(x)), int(std::floor(y)), int(std::floor(z)));
	if (!chunk || !chunk->isInitialized()) return false;
	DoubleBox box = definitions.entityRegistry().get<DoubleBox>(model);
	if (betaMobKind(id) == Kind::Slime) {
		MobComponent random; random.randomState = u32(m_seed) ^ u32(int(x)) ^ (u32(int(y)) * 31u) ^ m_mobTick;
		if (!slimeSize) slimeSize = u8(1 << (nextRandom(random) % 3));
		if (slimeSize != 1 && slimeSize != 2 && slimeSize != 4) return false;
		box.reset(-0.3 * slimeSize, -0.3 * slimeSize, 0, 0.6 * slimeSize, 0.6 * slimeSize, 0.6 * slimeSize);
	}
	box.move(x, y, z);
	for (int bz = int(std::floor(box.z)); bz <= int(std::floor(box.z + box.sizeZ)); ++bz)
		for (int by = int(std::floor(box.y)); by <= int(std::floor(box.y + box.sizeY)); ++by)
			for (int bx = int(std::floor(box.x)); bx <= int(std::floor(box.x + box.sizeX)); ++bx) {
				const BlockState *state = getBlockState(bx, by, bz);
				if (!state || (state->block().id() && state->isCollidable()
				 && collision::blockBox(*state, getData(bx, by, bz), bx, by, bz).intersects(box))) return false;
			}
	entt::entity entity = m_scene.createEntityFromModel(definitions.entityRegistry(), model);
	auto &registry = m_scene.registry();
	registry.emplace<PositionComponent>(entity, x, y, z, m_dimension.id());
	registry.emplace<NetworkComponent>(entity, entity);
	registry.emplace<std::string>(entity, id);
	auto &mob = registry.get<MobComponent>(entity);
	mob.randomState = u32(m_seed) ^ u32(entity) ^ 0x9e3779b9u;
	auto &traits = traitsFor(registry, entity);
	traits.eggTimer = 6000 + nextRandom(mob) % 6000;
	traits.jumpTimer = u16(10 + nextRandom(mob) % 20);
	traits.waypoint = {float(x), float(y), float(z)};
	if (traits.kind == Kind::Sheep) {
		u32 color = nextRandom(mob) % 100;
		traits.color = color < 5 ? 15 : color < 10 ? 7 : color < 15 ? 8 : color < 18 ? 12 : nextRandom(mob) % 500 == 0 ? 6 : 0;
	}
	syncAppearance(registry, entity);
	if (traits.kind == Kind::Slime) setSlimeSize(registry, entity, slimeSize, true);
	return true;
}

void ServerWorld::updateMobs() {
	auto &registry = m_scene.registry();
	registry.view<MobComponent>().each([&](auto entity, auto &) { traitsFor(registry, entity); });
	std::vector<entt::entity> dead;
	std::vector<Shot> shots;
	std::vector<Blast> blasts;
	registry.view<MobComponent, PositionComponent, DoubleBox, RotationComponent>().each(
		[&](auto entity, auto &mob, auto &position, auto &localBox, auto &rotation) {
			if (!mob.health) { dead.emplace_back(entity); return; }
			auto &traits = traitsFor(registry, entity);
			Kind kind = traits.kind;
			Vector3d origin{position.x, position.y, position.z};
			const Chunk *current = getChunkAtBlockPos(int(std::floor(position.x)), int(std::floor(position.y)), int(std::floor(position.z)));
			if (!current || !current->isInitialized()) {
				if (projectile(kind)) dead.emplace_back(entity);
				return;
			}
			++mob.age;
			if (mob.attackCooldown) --mob.attackCooldown;
			if (traits.anger) --traits.anger;
			if (projectile(kind)) {
				if (mob.age > 200) { dead.emplace_back(entity); return; }
				Vector3d velocity = traits.velocity;
				Vector3d end = origin + velocity;
				bool hit = !clearPath(*this, origin, end);
				for (const auto &entry : m_players) {
					const auto &player = entry.second;
					if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id() || player.name() == traits.owner) continue;
					DoubleBox box = player.hitbox(); box.move(player.x(), player.y(), player.z());
					double t = 1;
					if (segmentBox(origin, velocity, box, t) && clearPath(*this, origin, origin + velocity * t)) {
						end = origin + velocity * t; hit = true;
						if (kind == Kind::Arrow && m_server) m_server->damagePlayer(player.clientID(), 4);
						break;
					}
				}
				registry.view<MobComponent, PositionComponent, DoubleBox>().each([&](auto other, auto &otherMob, auto &otherPosition, auto &hitbox) {
					if (hit || other == entity || projectile(traitsFor(registry, other).kind)
					 || (mob.age < 5 && traits.owner == "mob:" + std::to_string(u32(other)))) return;
					DoubleBox box = hitbox; box.move(otherPosition.x, otherPosition.y, otherPosition.z);
					double t = 1;
					if (!segmentBox(origin, velocity, box, t)) return;
					end = origin + velocity * t; hit = true;
					if (kind == Kind::Arrow) otherMob.health = otherMob.health <= 4 ? 0 : otherMob.health - 4;
				});
				position.x = end.x; position.y = end.y; position.z = end.z; position.isUpdated = true;
				if (hit) {
					if (kind == Kind::Fireball) blasts.push_back({end, 1, nextRandom(mob)});
					dead.emplace_back(entity);
				}
				if (kind == Kind::Arrow) { traits.velocity *= 0.99f; traits.velocity.z -= 0.03f; }
				else traits.velocity = (traits.velocity + traits.waypoint) * 0.95f;
				rotation.quat = glm::angleAxis(float(std::atan2(velocity.y, velocity.x)
					+ (kind == Kind::Arrow ? 90 * math::DEG_TO_RAD : 0)), glm::vec3(0, 0, 1));
				rotation.isUpdated = true;
				return;
			}
			bool swimming = liquidAt(*this, origin + Vector3d{0, 0, localBox.sizeZ * 0.5});
			ServerPlayer *target = nullptr;
			double nearest = kind == Kind::Ghast ? 64 * 64 : 16 * 16;
			bool neutral = kind == Kind::Wolf || kind == Kind::PigZombie;
			bool sunlit = GameTime::getTicks() % GameTime::dayLength < 12000
				&& position.z >= m_heightmap.getHighestBlockAt(int(std::floor(position.x)), int(std::floor(position.y))) + 1;
			bool aggressive = mob.hostile || (neutral && traits.anger > 0);
			if (kind == Kind::Spider && sunlit && !traits.anger) aggressive = false;
			for (auto &entry : m_players) {
				auto &player = entry.second;
				if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
				bool following = kind == Kind::Wolf && traits.tamed && !traits.sitting && !traits.anger && player.name() == traits.owner;
				if (!aggressive && !following) continue;
				if (!traits.target.empty() && traits.target != player.name() && !following) continue;
				double dx = player.x() - position.x, dy = player.y() - position.y, dz = player.z() - position.z;
				double distance = dx * dx + dy * dy + dz * dz;
				if (distance < nearest && clearPath(*this, origin + Vector3d{0, 0, localBox.sizeZ * 0.8}, {player.x(), player.y(), player.z() + 1.5})) {
					target = &player; nearest = distance;
				}
			}
			if (target) mob.yaw = float(std::atan2(target->y() - position.y, target->x() - position.x));
			else if (mob.age % 60 == 1) mob.yaw = float(nextRandom(mob) % 6284) / 1000.f;
			bool attacksPlayer = target && aggressive && !(traits.tamed && target->name() == traits.owner);
			if (kind == Kind::Creeper) {
				if (advanceCreeperFuse(traits, std::sqrt(nearest), attacksPlayer)) {
					blasts.push_back({origin, 3, nextRandom(mob)}); dead.emplace_back(entity); return;
				}
			}
			else if (kind == Kind::Skeleton && attacksPlayer && nearest < 100 && !mob.attackCooldown) {
				Vector3d muzzle = origin + Vector3d{0, 0, 1.4};
				Vector3d direction{target->x() - position.x, target->y() - position.y, target->z() + 1.425 - muzzle.z};
				direction.z += std::sqrt(direction.x * direction.x + direction.y * direction.y) * 0.2;
				shots.push_back({muzzle, direction, Kind::Arrow, entity}); mob.attackCooldown = 30;
			}
			else if (kind == Kind::Ghast) {
				if (attacksPlayer) {
					if (++traits.attackCharge >= 20) {
						Vector3d muzzle = origin + Vector3d{std::cos(mob.yaw) * 4, std::sin(mob.yaw) * 4, 2.5};
						shots.push_back({muzzle, Vector3d{target->x(), target->y(), target->z() + 0.9} - muzzle, Kind::Fireball, entity});
						traits.attackCharge = -40;
					}
				}
				else if (traits.attackCharge > 0) --traits.attackCharge;
			}
			else if (attacksPlayer && mob.attackCooldown == 0) {
				double range = kind == Kind::Wolf ? 1.5 : kind == Kind::Slime ? traits.size * 0.6 : 2;
				if (nearest < range * range && (kind != Kind::Slime || traits.size > 1)) {
					mob.attackCooldown = 20;
					if (m_server) m_server->damagePlayer(target->clientID(), kind == Kind::Wolf && traits.tamed ? 4 : mob.attackStrength);
				}
			}
			bool walking = target || (mob.age % 100 < 60);
			if (kind == Kind::Wolf && (traits.sitting || (traits.tamed && target && nearest < 25))) walking = false;
			if (kind == Kind::Creeper && traits.fuse) walking = false;
			if (kind == Kind::Skeleton && target && nearest < 36) walking = false;
			float speed = walking ? mob.moveSpeed * 0.2f : 0;
			if (kind == Kind::PigZombie && traits.anger) speed = walking ? 0.19f : 0;
			mob.verticalSpeed = std::max(-0.5f, (mob.verticalSpeed - 0.08f) * 0.98f);
			if (kind == Kind::Chicken && mob.verticalSpeed < 0) mob.verticalSpeed *= 0.6f;
			if (swimming && kind != Kind::Squid) mob.verticalSpeed = std::max(mob.verticalSpeed, 0.08f);
			Vector3d desired{std::cos(mob.yaw) * speed, std::sin(mob.yaw) * speed, mob.verticalSpeed};
			if (kind == Kind::Slime) {
				if (traits.grounded) {
					if (traits.jumpTimer) --traits.jumpTimer;
					else {
						traits.jumpTimer = u16((10 + nextRandom(mob) % 20) / (target ? 3 : 1));
						mob.verticalSpeed = 0.42f; desired.z = mob.verticalSpeed;
					}
					if (traits.jumpTimer) desired.x = desired.y = 0;
				}
			}
			if ((kind == Kind::Spider || kind == Kind::Wolf) && target && nearest > 4 && nearest < 36 && traits.grounded && nextRandom(mob) % 10 == 0)
				desired.z = mob.verticalSpeed = 0.4f;
			if (kind == Kind::Ghast || (kind == Kind::Squid && swimming)) {
				if (mob.age % (kind == Kind::Ghast ? 20 : 50) == 1 || traits.velocity.isZero()) {
					traits.velocity = {float(std::cos(mob.yaw) * 0.2), float(std::sin(mob.yaw) * 0.2), float(unitRandom(mob) * 0.2 - 0.1)};
				}
				desired = traits.velocity;
			}
			if (kind == Kind::Squid && !swimming) desired.x = desired.y = 0;
			DoubleBox box = localBox; box.move(position.x, position.y, position.z);
			Vector3d movement = collision::move(*this, box, desired);
			traits.grounded = desired.z < 0 && movement.z > desired.z;
			if (movement.z != desired.z) mob.verticalSpeed = 0;
			bool blocked = std::abs(movement.x - desired.x) > 0.001 || std::abs(movement.y - desired.y) > 0.001;
			if (kind == Kind::Spider && blocked) mob.verticalSpeed = 0.2f;
			else if (traits.grounded && blocked) mob.verticalSpeed = 0.42f;
			position.x += movement.x; position.y += movement.y; position.z += movement.z;
			position.isUpdated = position.isUpdated || !movement.isZero();
			rotation.quat = glm::angleAxis(mob.yaw + float(M_PI / 2), glm::vec3(0, 0, 1)); rotation.isUpdated = true;
			if ((kind == Kind::Zombie || kind == Kind::Skeleton) && mob.age % 20 == 0 && sunlit) --mob.health;
			if (kind == Kind::Chicken) {
				if (!traits.eggTimer) traits.eggTimer = 6000 + nextRandom(mob) % 6000;
				if (--traits.eggTimer == 0) dropItem(*this, origin, "default:egg", 1);
			}
			syncAppearance(registry, entity);
			if (!mob.health) dead.emplace_back(entity);
		});
	for (const auto &blast : blasts) explode(*this, blast);
	registry.view<MobComponent>().each([&](auto entity, auto &mob) { if (!mob.health) dead.emplace_back(entity); });
	std::sort(dead.begin(), dead.end()); dead.erase(std::unique(dead.begin(), dead.end()), dead.end());
	for (auto entity : dead) {
		if (!registry.valid(entity)) continue;
		auto traits = traitsFor(registry, entity);
		auto position = registry.get<PositionComponent>(entity);
		const auto &mob = registry.get<MobComponent>(entity);
		if (!projectile(traits.kind) && !(traits.kind == Kind::Creeper && traits.fuse >= 30)) {
			auto &random = registry.get<MobComponent>(entity);
			u16 count = u16(nextRandom(random) % 3);
			if (traits.kind == Kind::Sheep) count = traits.sheared ? 0 : 1;
			if (traits.kind == Kind::Squid) count = u16(1 + nextRandom(random) % 3);
			if (traits.kind == Kind::Slime && traits.size > 1) count = 0;
			dropItem(*this, {position.x, position.y, position.z}, mob.dropItem, count);
			if (traits.kind == Kind::Skeleton)
				dropItem(*this, {position.x, position.y, position.z}, "default:bone", u16(nextRandom(random) % 3));
		}
		if (m_server) m_server->sendEntityDespawn(entity);
		registry.destroy(entity);
		if (traits.kind == Kind::Slime && traits.size > 1) {
			for (int child = 0; child < 4; ++child) {
				double x = position.x + ((child % 2) - 0.5) * traits.size / 4;
				double y = position.y + ((child / 2) - 0.5) * traits.size / 4;
				spawnMob("default:slime", x, y, position.z + 0.5, u8(traits.size / 2));
			}
		}
	}
	for (const auto &shot : shots) spawnShot(*this, shot);
	if (++m_mobTick % 200 != 0) return;
	bool night = GameTime::getTicks() % GameTime::dayLength >= 12000;
	static const char *passive[] = {"pig", "cow", "sheep", "chicken", "wolf"};
	static const char *hostile[] = {"zombie", "skeleton", "creeper", "spider"};
	for (const auto &entry : m_players) {
		const auto &player = entry.second;
		if (!player.isReady() || !player.isOnline() || player.dimension() != m_dimension.id()) continue;
		std::uint64_t spawnSeed = std::uint64_t(std::int64_t(m_seed)) + std::uint64_t(m_mobTick) * 341873128712ULL
			+ std::uint64_t(player.clientID()) * 132897987541ULL;
		retro::Random random(std::int64_t(spawnSeed & ((1ULL << 48) - 1)));
		int x = int(std::floor(player.x())) + random.nextInt(65) - 32;
		int y = int(std::floor(player.y())) + random.nextInt(65) - 32;
		double dx = x + 0.5 - player.x(), dy = y + 0.5 - player.y();
		if (dx * dx + dy * dy < 24 * 24) continue;
		if (m_dimension.id() == 1) {
			const char *name = m_mobTick % 400 ? "default:pig_zombie" : "default:ghast";
			auto model = Registry::getInstance().getEntityFromStringID(name);
			if (model == entt::null) continue;
			const auto *box = Registry::getInstance().entityRegistry().try_get<DoubleBox>(model);
			if (!box) continue;
			int floor = retro::mobCavityFloor(*this, *box, x + 0.5, y + 0.5, int(std::floor(player.z())));
			if (floor >= 0) spawnMob(name, x + 0.5, y + 0.5, floor);
			continue;
		}
		if (m_dimension.id() != 0) continue;
		int cx = int(std::floor(x / 16.0)), cy = int(std::floor(y / 16.0));
		if (random.nextInt(10) == 0 && retro::slimeChunk(m_seed, cx, cy)) {
			u8 size = u8(1 << random.nextInt(3));
			DoubleBox box{-0.3 * size, -0.3 * size, 0, 0.6 * size, 0.6 * size, 0.6 * size};
			int floor = retro::mobCavityFloor(*this, box, x + 0.5, y + 0.5, 1 + random.nextInt(15), 15, 15);
			if (floor >= 0) spawnMob("default:slime", x + 0.5, y + 0.5, floor, size);
		}
		int z = m_heightmap.getHighestBlockAt(x, y) + 1;
		const BlockState *ground = getBlockState(x, y, z - 1);
		if (z < SEALEVEL && liquidAt(*this, {x + 0.5, y + 0.5, SEALEVEL - 3.0})) {
			spawnMob("default:squid", x + 0.5, y + 0.5, SEALEVEL - 3.0); continue;
		}
		if (!ground || z <= SEALEVEL) continue;
		if (night) spawnMob("default:" + std::string(hostile[(m_mobTick / 200) % 4]), x + 0.5, y + 0.5, z);
		else if (ground->block().stringID() == "default:grass") {
			const char *name = passive[(m_mobTick / 200) % 5];
			if (std::string(name) == "wolf") {
				auto biome = m_heightmap.getOrCreateChunk(int(std::floor(x / 16.0)), int(std::floor(y / 16.0))).biomeAt(x & 15, y & 15);
				if (biome != retro::Biome::Taiga && biome != retro::Biome::Forest) continue;
			}
			spawnMob("default:" + std::string(name), x + 0.5, y + 0.5, z);
		}
	}
}

entt::entity ServerWorld::findMobTarget(ServerPlayer &player, double reach) {
	Vector3d origin{player.x(), player.y(), player.z() + 1.625};
	double yaw = player.cameraYaw() * math::DEG_TO_RAD, pitch = player.cameraPitch() * math::DEG_TO_RAD;
	Vector3d direction{std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch), std::sin(pitch)};
	entt::entity selected = entt::null;
	m_scene.registry().view<MobComponent, PositionComponent, DoubleBox>().each([&](auto entity, auto &, auto &position, auto &hitbox) {
		DoubleBox box = hitbox; box.move(position.x, position.y, position.z);
		double distance = reach;
		if (segmentBox(origin, direction, box, distance) && distance < reach && clearPath(*this, origin, origin + direction * distance)) {
			reach = distance; selected = entity;
		}
	});
	return selected;
}

bool ServerWorld::interactMob(ServerPlayer &player) {
	entt::entity entity = findMobTarget(player, 3);
	if (entity == entt::null) return false;
	auto &registry = m_scene.registry();
	auto &traits = traitsFor(registry, entity);
	auto &mob = registry.get<MobComponent>(entity);
	ItemStack held;
	if (player.heldItemSlot() >= 0 && player.heldItemSlot() < player.inventory().width()) held = player.heldItemStack();
	std::string item = held.amount() ? held.item().stringID() : "";
	bool handled = false;
	if (traits.kind == Kind::Pig && item == "default:saddle" && !traits.saddled && consumeFor(player, "")) {
		traits.saddled = true; handled = true;
	}
	else if (traits.kind == Kind::Cow && item == "default:bucket_empty") handled = consumeFor(player, "default:bucket_milk");
	else if (traits.kind == Kind::Sheep && item == "default:shears" && !traits.sheared) {
		traits.sheared = true;
		auto &position = registry.get<PositionComponent>(entity);
		dropItem(*this, {position.x, position.y, position.z}, "default:wool", u16(2 + nextRandom(mob) % 3));
		player.inventory().getStackRef(player.heldItemSlot(), 0).wear(1);
		handled = true;
	}
	else if (traits.kind == Kind::Wolf) {
		if (!traits.tamed && !traits.anger && item == "default:bone" && consumeFor(player, "")) {
			if (nextRandom(mob) % 3 == 0) { traits.tamed = true; traits.sitting = true; traits.owner = player.name(); mob.health = 20; }
			handled = true;
		}
		else if (traits.tamed && traits.owner == player.name()) {
			if ((item == "default:porkchop_raw" || item == "default:porkchop_cooked") && mob.health < 20 && consumeFor(player, ""))
				mob.health = std::min<u16>(20, mob.health + 3);
			else traits.sitting = !traits.sitting;
			handled = true;
		}
	}
	if (handled) syncAppearance(registry, entity);
	if (handled && m_server) m_server->sendPlayerInvUpdate(player.clientID());
	return handled;
}

bool ServerWorld::dropWorldItem(Vector3d position, const std::string &id, u16 count) {
	return dropItem(*this, position, id, count);
}

void ServerWorld::dropPlayerItems(ServerPlayer &player) {
	for (u16 y = 0; y < player.inventory().height(); ++y) {
		for (u16 x = 0; x < player.inventory().width(); ++x) {
			ItemStack stack = player.inventory().getStack(x, y);
			if (stack.amount() && dropItem(*this, {player.x(), player.y(), player.z()}, stack.item().stringID(), stack.amount(), stack.damage()))
				player.inventory().clearStack(x, y);
		}
	}
	if (m_server) m_server->sendPlayerInvUpdate(player.clientID());
}

bool ServerWorld::attackMob(ServerPlayer &player) {
	entt::entity selected = findMobTarget(player, 3);
	if (selected == entt::null) return false;
	auto &registry = m_scene.registry();
	registry.view<MobComponent>().each([&](auto entity, auto &) { traitsFor(registry, entity); });
	auto &mob = registry.get<MobComponent>(selected);
	auto &traits = traitsFor(registry, selected);
	auto position = registry.get<PositionComponent>(selected);
	if (traits.kind == Kind::Fireball) {
		double yaw = player.cameraYaw() * math::DEG_TO_RAD, pitch = player.cameraPitch() * math::DEG_TO_RAD;
		traits.velocity = {float(std::cos(yaw) * std::cos(pitch)), float(std::sin(yaw) * std::cos(pitch)), float(std::sin(pitch))};
		traits.waypoint = traits.velocity * 0.1f; traits.owner = player.name(); return true;
	}
	if (projectile(traits.kind)) return false;
	if (traits.kind == Kind::Spider) { traits.anger = 400; traits.target = player.name(); }
	if (traits.kind == Kind::PigZombie || traits.kind == Kind::Wolf) {
		registry.view<MobComponent, PositionComponent>().each([&](auto entity, auto &otherMob, auto &otherPosition) {
			auto &other = traitsFor(registry, entity);
			if (other.kind != traits.kind || (other.tamed && other.owner == player.name())) return;
			double dx = otherPosition.x - position.x, dy = otherPosition.y - position.y, dz = otherPosition.z - position.z;
			double radius = traits.kind == Kind::PigZombie ? 32 : 16;
			if (dx * dx + dy * dy + dz * dz <= radius * radius) {
				other.anger = 400 + nextRandom(otherMob) % 400; other.target = player.name(); other.sitting = false;
				syncAppearance(registry, entity);
			}
		});
	}
	u16 damage = 1;
	if (player.heldItemSlot() >= 0 && player.heldItemSlot() < player.inventory().width()) {
		auto &tool = player.inventory().getStackRef(player.heldItemSlot(), 0);
		damage = retro::attackDamage(tool.item());
		if (tool.wear(retro::attackingWear(tool.item())) && m_server) m_server->sendPlayerInvUpdate(player.clientID());
	}
	mob.health = damage >= mob.health ? 0 : mob.health - damage;

	return true;
}
