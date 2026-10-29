#ifndef PHYSICALCOMPONENT_HPP_
#define PHYSICALCOMPONENT_HPP_

#include "common/core/Vector3.hpp"
#include "common/network/Network.hpp"

struct PhysicalComponent {
	Vector3f velocity{0, 0, 2};
	u32 age = 0;
	u16 pickupDelay = 10;
	Vector3i forceOrigin{0, 0, 0};
	Vector3i forceDirection{0, 0, 0};
};

inline sf::Packet &operator<<(sf::Packet &packet, const PhysicalComponent &body) {
	return packet << body.velocity.x << body.velocity.y << body.velocity.z << body.age << body.pickupDelay
		<< body.forceOrigin.x << body.forceOrigin.y << body.forceOrigin.z
		<< body.forceDirection.x << body.forceDirection.y << body.forceDirection.z;
}
inline sf::Packet &operator>>(sf::Packet &packet, PhysicalComponent &body) {
	return packet >> body.velocity.x >> body.velocity.y >> body.velocity.z >> body.age >> body.pickupDelay
		>> body.forceOrigin.x >> body.forceOrigin.y >> body.forceOrigin.z
		>> body.forceDirection.x >> body.forceDirection.y >> body.forceDirection.z;
}

#endif // PHYSICALCOMPONENT_HPP_
