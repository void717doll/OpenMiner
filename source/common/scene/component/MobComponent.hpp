#ifndef MOBCOMPONENT_HPP_
#define MOBCOMPONENT_HPP_

#include "common/core/Vector3.hpp"
#include "common/network/Network.hpp"

struct MobComponent {
	bool hostile = false;
	u16 health = 10;
	float moveSpeed = 0.7f;
	u16 attackStrength = 0;
	std::string dropItem;
	float yaw = 0;
	float verticalSpeed = 0;
	u32 age = 0;
	u32 attackCooldown = 0;
	u32 randomState = 1;
};

inline sf::Packet &operator<<(sf::Packet &packet, const MobComponent &mob) {
	return packet << mob.hostile << mob.health << mob.moveSpeed << mob.attackStrength << mob.dropItem
		<< mob.yaw << mob.verticalSpeed << mob.age << mob.attackCooldown << mob.randomState;
}
inline sf::Packet &operator>>(sf::Packet &packet, MobComponent &mob) {
	return packet >> mob.hostile >> mob.health >> mob.moveSpeed >> mob.attackStrength >> mob.dropItem
		>> mob.yaw >> mob.verticalSpeed >> mob.age >> mob.attackCooldown >> mob.randomState;
}

#endif // MOBCOMPONENT_HPP_
