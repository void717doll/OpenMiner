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


// kept separate so saves containing the original mob component remain readable.
enum class BetaMobKind : u8 {
	Unknown, Pig, Zombie, Cow, Sheep, Chicken, Skeleton, Creeper, Spider,
	Slime, Ghast, PigZombie, Wolf, Squid, Giant, Arrow, Fireball
};

inline const char *betaMobName(BetaMobKind kind) {
	static const char *names[] = {"", "pig", "zombie", "cow", "sheep", "chicken", "skeleton", "creeper",
		"spider", "slime", "ghast", "pig_zombie", "wolf", "squid", "giant", "arrow_projectile", "fireball_projectile"};
	u8 index = u8(kind);
	return index < 17 ? names[index] : "";
}

inline BetaMobKind betaMobKind(const std::string &name) {
	std::string local = name.substr(name.find(':') == std::string::npos ? 0 : name.find(':') + 1);
	for (u8 kind = 1; kind < 17; ++kind)
		if (local == betaMobName(BetaMobKind(kind))) return BetaMobKind(kind);
	return BetaMobKind::Unknown;
}

struct BetaMobComponent {
	BetaMobKind kind = BetaMobKind::Unknown;
	u8 size = 1;
	u16 fuse = 0;
	u32 anger = 0;
	u32 eggTimer = 0;
	u16 jumpTimer = 0;
	s16 attackCharge = 0;
	bool grounded = false;
	bool sheared = false;
	bool tamed = false;
	bool sitting = false;
	u8 color = 0;
	bool saddled = false;
	std::string owner;
	std::string target;
	Vector3f velocity{0, 0, 0};
	Vector3f waypoint{0, 0, 0};
};

inline sf::Packet &operator<<(sf::Packet &packet, const BetaMobComponent &mob) {
	return packet << u8(mob.kind) << mob.size << mob.fuse << mob.anger << mob.eggTimer << mob.jumpTimer
		<< mob.attackCharge << mob.grounded << mob.sheared << mob.tamed << mob.sitting << mob.color << mob.saddled << mob.owner << mob.target
		<< mob.velocity.x << mob.velocity.y << mob.velocity.z << mob.waypoint.x << mob.waypoint.y << mob.waypoint.z;
}
inline sf::Packet &operator>>(sf::Packet &packet, BetaMobComponent &mob) {
	u8 kind;
	packet >> kind >> mob.size >> mob.fuse >> mob.anger >> mob.eggTimer >> mob.jumpTimer
		>> mob.attackCharge >> mob.grounded >> mob.sheared >> mob.tamed >> mob.sitting >> mob.color >> mob.saddled >> mob.owner >> mob.target
		>> mob.velocity.x >> mob.velocity.y >> mob.velocity.z >> mob.waypoint.x >> mob.waypoint.y >> mob.waypoint.z;
	mob.kind = kind < 17 ? BetaMobKind(kind) : BetaMobKind::Unknown;
	return packet;
}

inline bool advanceCreeperFuse(BetaMobComponent &mob, double distance, bool targetVisible) {
	bool ignited = targetVisible && (distance < 3 || (mob.fuse > 0 && distance < 7));
	if (ignited) ++mob.fuse;
	else if (mob.fuse) --mob.fuse;
	return mob.fuse >= 30;
}

#endif // MOBCOMPONENT_HPP_
