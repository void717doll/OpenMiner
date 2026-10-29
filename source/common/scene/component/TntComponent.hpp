#ifndef TNTCOMPONENT_HPP_
#define TNTCOMPONENT_HPP_

#include "common/network/Network.hpp"
#include "common/network/NetworkUtils.hpp"

struct TntComponent : public ISerializable {
	u16 age = 0, lifetime = 80;
	Vector3d velocity{0, 0, 4};
	u32 seed = 1;
	bool isUpdated = false;
	Network::Command packetType{};

	void serialize(sf::Packet &packet) const override {
		packet << age << lifetime << velocity << seed;
	}
	void deserialize(sf::Packet &packet) override {
		packet >> age >> lifetime >> velocity >> seed;
	}
};

#endif // TNTCOMPONENT_HPP_
