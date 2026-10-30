#include "common/world/WorldBounds.hpp"
#include <cmath>

#include "common/core/GameTime.hpp"
#include "common/core/Registry.hpp"
#include "common/scene/component/MobComponent.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/world/Dimension.hpp"
#include "common/world/WorldCollision.hpp"
#include "server/core/PlayerList.hpp"
#include "server/world/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

namespace {

constexpr Vector3i directions[] = {{1, 0, 0}, {0, 1, 0}, {-1, 0, 0}, {0, -1, 0}};

bool loaded(const ServerWorld &world, Vector3i position) {
	const Chunk *chunk = world.getChunkAtBlockPos(position.x, position.y, position.z);
	return chunk && chunk->isInitialized();
}

bool solidFloor(const ServerWorld &world, Vector3i position) {
	if (!loaded(world, position)) return false;
	const auto *state = world.getBlockState(position.x, position.y, position.z);
	return state && state->block().id() && state->isCollidable() && state->boundingBox().sizeZ == 1
		&& state->boundingBox().sizeX == 1 && state->boundingBox().sizeY == 1
		&& state->block().stringID() != "default:cactus";
}

bool bedPart(const ServerWorld &world, Vector3i position, Vector3i &partner, bool &head, u16 &rotation) {
	if (!loaded(world, position)) return false;
	const auto *state = world.getBlockState(position.x, position.y, position.z);
	if (!state || state->block().stringID() != "default:bed") return false;
	u16 data = world.getData(position.x, position.y, position.z);
	rotation = state->block().param().getParam(BlockParam::Rotation, data);
	if (rotation > 3 || state->id() > 1) return false;
	head = state->id() == 1;
	partner = position + directions[rotation] * (head ? -1 : 1);
	return true;
}

bool pair(const ServerWorld &world, Vector3i position, Vector3i &foot, Vector3i &head) {
	Vector3i partner, other;
	bool firstHead, secondHead;
	u16 rotation, otherRotation;
	if (!bedPart(world, position, partner, firstHead, rotation)
	 || !bedPart(world, partner, other, secondHead, otherRotation)
	 || other != position || firstHead == secondHead || rotation != otherRotation) return false;
	foot = firstHead ? partner : position;
	head = firstHead ? position : partner;
	return true;
}

}

bool ServerWorld::placeBed(ServerPlayer &player, Vector3i foot, u16 &data) {
	const Block &bed = Registry::getInstance().getBlockFromStringID("default:bed");
	u16 rotation = player.getDirection();
	Vector3i head = foot + directions[rotation];
	for (Vector3i position : {foot, head}) {
		if (!worldBounds::contains(position.z) || !loaded(*this, position) || getBlock(position.x, position.y, position.z)
		 || !solidFloor(*this, position + Vector3i{0, 0, -1})) return false;
		DoubleBox box{double(position.x), double(position.y), double(position.z), 1, 1, 9.0 / 16};
		DoubleBox playerBox = player.hitbox();
		playerBox.move(player.x(), player.y(), player.z());
		if (box.intersects(playerBox)) return false;
		for (const auto &entry : players()) {
			const auto &other = entry.second;
			if (&other == &player || !other.isOnline() || other.dimension() != dimension().id()) continue;
			DoubleBox otherBox = other.hitbox();
			otherBox.move(other.x(), other.y(), other.z());
			if (box.intersects(otherBox)) return false;
		}
	}
	data = bed.param().setParam(BlockParam::Rotation, 0, rotation);
	setData(foot.x, foot.y, foot.z, data);
	setBlock(foot.x, foot.y, foot.z, bed.id());
	setData(head.x, head.y, head.z, bed.param().setParam(BlockParam::State, data, 1));
	setBlock(head.x, head.y, head.z, bed.id());
	return true;
}

void ServerWorld::removeBedPartner(int x, int y, int z) {
	auto &definitions = Registry::getInstance();
	if (!definitions.hasBlock("default:bed")) return;
	const auto &bed = definitions.getBlockFromStringID("default:bed");
	u16 data = getData(x, y, z);
	u16 rotation = bed.param().getParam(BlockParam::Rotation, data);
	u16 half = bed.param().getParam(BlockParam::State, data);
	if (rotation > 3 || half > 1) return;
	Vector3i partner = Vector3i{x, y, z} + directions[rotation] * (half ? -1 : 1);
	if (!loaded(*this, partner)) return;
	const auto *state = getBlockState(partner.x, partner.y, partner.z);
	if (!state || state->block().id() != bed.id() || state->id() == half) return;
	if (bed.param().getParam(BlockParam::Rotation, getData(partner.x, partner.y, partner.z)) != rotation) return;
	setBlock(partner.x, partner.y, partner.z, 0);
}

void ServerWorld::validateBed(int x, int y, int z) {
	Vector3i position{x, y, z}, partner, foot, head;
	bool isHead;
	u16 rotation;
	if (!bedPart(*this, position, partner, isHead, rotation) || !loaded(*this, partner)
	 || !loaded(*this, position + Vector3i{0, 0, -1}) || !loaded(*this, partner + Vector3i{0, 0, -1})) return;
	if (pair(*this, position, foot, head) && solidFloor(*this, foot + Vector3i{0, 0, -1})
	 && solidFloor(*this, head + Vector3i{0, 0, -1})) return;
	setBlock(x, y, z, 0);
	dropWorldItem({x + 0.5, y + 0.5, z + 0.25}, "default:bed", 1);
}

bool ServerWorld::bedRespawn(Vector3i bed, Vector3d &arrival, bool loadChunks) {
	if (dimension().id() != 0) return false;
	if (loadChunks) {
		for (int z = bed.z - 1; z <= bed.z + 1; ++z)
			for (int y = bed.y - 2; y <= bed.y + 2; ++y)
				for (int x = bed.x - 2; x <= bed.x + 2; ++x) {
					int cx = int(std::floor(x / double(CHUNK_WIDTH))), cy = int(std::floor(y / double(CHUNK_DEPTH)));
					int cz = int(std::floor(z / double(CHUNK_HEIGHT)));
					auto &chunk = getOrCreateChunk(cx, cy, cz);
					if (!chunk.isInitialized()) generateChunk(chunk);
				}
	}
	Vector3i foot, head;
	if (!pair(*this, bed, foot, head) || !solidFloor(*this, foot + Vector3i{0, 0, -1})
	 || !solidFloor(*this, head + Vector3i{0, 0, -1})) return false;
	for (Vector3i half : {head, foot}) {
		for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) {
			Vector3i point = half + Vector3i{dx, dy, 0};
			if (!loaded(*this, point) || !loaded(*this, point + Vector3i{0, 0, 1})
			 || !solidFloor(*this, point + Vector3i{0, 0, -1})
			 || getBlock(point.x, point.y, point.z) || getBlock(point.x, point.y, point.z + 1)) continue;
			DoubleBox box{point.x + 0.2, point.y + 0.2, double(point.z), 0.6, 0.6, 1.8};
			std::vector<DoubleBox> obstacles;
			appendCollisionBoxes(box, box, obstacles);
			if (std::any_of(obstacles.begin(), obstacles.end(), [&](const auto &obstacle) { return obstacle.intersects(box); })) continue;
			arrival = {point.x + 0.5, point.y + 0.5, double(point.z)};
			return true;
		}
	}
	return false;
}

std::string ServerWorld::sleepInBed(ServerPlayer &player, Vector3i position) {
	Vector3i foot, head;
	if (!pair(*this, position, foot, head)) return "this bed is incomplete";
	if (dimension().id() != 0) {
		setBlock(foot.x, foot.y, foot.z, 0);
		explodeAt({head.x + 0.5, head.y + 0.5, head.z + 0.5}, 5, u32(GameTime::getTicks()), "bed");
		return "";
	}
	if (!player.health() || std::abs(player.x() - head.x) > 3 || std::abs(player.y() - head.y) > 3
	 || std::abs(player.z() - head.z) > 2) return "move closer to the bed";
	if (GameTime::getTicks() % GameTime::dayLength < 12000) return "you can only sleep at night";
	bool danger = false;
	scene().registry().view<MobComponent, PositionComponent>().each([&](auto, const auto &mob, const auto &mobPosition) {
		if (mob.hostile && mob.health && std::abs(mobPosition.x - head.x) <= 8
		 && std::abs(mobPosition.y - head.y) <= 8 && std::abs(mobPosition.z - head.z) <= 5) danger = true;
	});
	if (danger) return "you cannot sleep while monsters are nearby";
	Vector3d arrival;
	if (!bedRespawn(head, arrival)) return "this bed is obstructed";
	player.setBedSpawn(head);
	player.setPosition(arrival.x, arrival.y, arrival.z);
	player.resetSurvival();
	GameTime::setTicks((GameTime::getTicks() / GameTime::dayLength + 1) * GameTime::dayLength);
	return "respawn point set";
}
