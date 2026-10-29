#ifndef PLAYERSURVIVAL_HPP_
#define PLAYERSURVIVAL_HPP_

#include <algorithm>
#include <cmath>

#include "common/world/WorldCollision.hpp"
#include "common/world/LiquidState.hpp"

struct PlayerEnvironment {
	bool loaded = true;
	bool grounded = false;
	bool water = false;
	bool submerged = false;
	bool lava = false;
	bool fire = false;
	bool cactus = false;
	bool suffocating = false;
};

struct PlayerSurvivalState {
	double fallDistance = 0;
	s16 air = 300;
	s16 fireTicks = -20;
	u8 hurtTicks = 0;
	u16 lastDamage = 0;

	u16 filterDamage(u16 amount) {
		if (!amount) return 0;
		if (hurtTicks > 10) {
			if (amount <= lastDamage) return 0;
			u16 difference = amount - lastDamage;
			lastDamage = amount;
			return difference;
		}
		hurtTicks = 20;
		lastDamage = amount;
		return amount;
	}

	u16 moved(double dz, bool grounded, bool inWater) {
		if (inWater) { fallDistance = 0; return 0; }
		if (dz < 0) fallDistance -= dz;
		if (!grounded) return 0;
		double damage = std::ceil(fallDistance - 3);
		fallDistance = 0;
		return u16(std::max(0.0, std::min(65535.0, damage)));
	}

	u16 tick(const PlayerEnvironment &environment) {
		if (!environment.loaded) { fallDistance = 0; return 0; }
		if (hurtTicks) --hurtTicks;
		u16 damage = 0;
		if (environment.water) { fallDistance = 0; fireTicks = -20; }
		if (fireTicks > 0) {
			if (fireTicks % 20 == 0) damage = 1;
			--fireTicks;
		}
		if (environment.lava) { damage = 4; fireTicks = 600; }
		if (environment.suffocating || environment.cactus) damage = std::max<u16>(damage, 1);
		if (environment.submerged) {
			if (--air == -20) { air = 0; damage = std::max<u16>(damage, 2); }
		} else air = 300;
		if (environment.fire) {
			damage = std::max<u16>(damage, 1);
			if (++fireTicks == 0) fireTicks = 300;
		} else if (fireTicks <= 0) fireTicks = -20;
		if (environment.water || environment.submerged) fireTicks = -20;
		return damage;
	}
};

inline PlayerEnvironment playerEnvironment(const World &world, const DoubleBox &box) {
	PlayerEnvironment environment;
	DoubleBox waterBox = box;
	waterBox.z += 0.4;
	waterBox.sizeZ = std::max(0.0, waterBox.sizeZ - 0.8);
	DoubleBox lavaBox = waterBox;
	lavaBox.x += 0.1; lavaBox.y += 0.1;
	lavaBox.sizeX = std::max(0.0, lavaBox.sizeX - 0.2);
	lavaBox.sizeY = std::max(0.0, lavaBox.sizeY - 0.2);
	double eyeX = box.x + box.sizeX / 2, eyeY = box.y + box.sizeY / 2, eyeZ = box.z + 1.625;
	for (int z = int(std::floor(box.z - 0.05)); z < int(std::ceil(box.z + box.sizeZ)); ++z)
		for (int y = int(std::floor(box.y)); y < int(std::ceil(box.y + box.sizeY)); ++y)
			for (int x = int(std::floor(box.x)); x < int(std::ceil(box.x + box.sizeX)); ++x) {
				const Chunk *chunk = world.getChunkAtBlockPos(x, y, z);
				const BlockState *state = world.getBlockState(x, y, z);
				if (!chunk || !chunk->isInitialized() || !state) { environment.loaded = false; continue; }
				const std::string &id = state->block().stringID();
				DoubleBox cell{double(x), double(y), double(z), 1, 1, 1};
				if (liquid::kind(state->block())) {
					const BlockState *above = world.getBlockState(x, y, z + 1);
					bool covered = above && liquid::kind(above->block()) == liquid::kind(state->block());
					cell.sizeZ = liquid::height(state->block(), world.getData(x, y, z), covered);
				}
				bool bodyContact = cell.intersects(box);
				if (liquid::kind(state->block()) == 1) {
					environment.water |= cell.intersects(waterBox);
					environment.submerged |= int(std::floor(eyeX)) == x && int(std::floor(eyeY)) == y
						&& int(std::floor(eyeZ)) == z && eyeZ < z + cell.sizeZ;
				}
				if (liquid::kind(state->block()) == 2) environment.lava |= cell.intersects(lavaBox);
				if (id == "default:fire") environment.fire |= bodyContact;
				if (id == "default:cactus") environment.cactus |= bodyContact;
				if (state->block().id() && state->isOpaque() && state->isCollidable()) {
					DoubleBox solid = collision::blockBox(*state, world.getData(x, y, z), x, y, z);
					environment.suffocating |= eyeX > solid.x && eyeX < solid.x + solid.sizeX
						&& eyeY > solid.y && eyeY < solid.y + solid.sizeY && eyeZ > solid.z && eyeZ < solid.z + solid.sizeZ;
				}
			}
	if (environment.loaded) environment.grounded = collision::move(world, box, {0, 0, -0.05}).z > -0.049;
	return environment;
}

#endif // PLAYERSURVIVAL_HPP_
