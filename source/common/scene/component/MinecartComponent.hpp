#ifndef MINECARTCOMPONENT_HPP_
#define MINECARTCOMPONENT_HPP_

#include <cmath>
#include <string>

#include "common/core/ISerializable.hpp"
#include "common/network/Network.hpp"
#include "common/network/NetworkUtils.hpp"

struct MinecartComponent : public ISerializable {
	// beta velocities are blocks per 50 ms tick.
	Vector3d velocity{0, 0, 0};
	u16 damage = 0;
	std::string rider;
	bool isUpdated = false;
	Network::Command packetType{};

	void serialize(sf::Packet &packet) const override { packet << velocity << damage; }
	void deserialize(sf::Packet &packet) override {
		packet >> velocity >> damage;
		// occupancy is a session attachment, never a saved player reference.
		rider.clear();
		if (!std::isfinite(velocity.x) || !std::isfinite(velocity.y) || !std::isfinite(velocity.z))
			velocity = {0, 0, 0};
	}
};
#endif
