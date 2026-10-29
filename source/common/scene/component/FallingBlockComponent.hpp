#ifndef FALLINGBLOCKCOMPONENT_HPP_
#define FALLINGBLOCKCOMPONENT_HPP_

#include <optional>

#include "common/network/Network.hpp"
#include "common/network/NetworkUtils.hpp"
#include "common/world/BlockData.hpp"

struct FallingBlockComponent : public ISerializable {
	u16 block = 0;
	u16 data = 0;
	Vector3f velocity{0, 0, 0};
	Vector3i origin{0, 0, 0};
	std::optional<BlockData> blockData;

	void serialize(sf::Packet &packet) const override {
		packet << block << data << velocity << origin << bool(blockData);
		if (blockData) packet << blockData->inventory << blockData->meta;
	}
	void deserialize(sf::Packet &packet) override {
		bool hasMetadata = false;
		packet >> block >> data >> velocity >> origin >> hasMetadata;
		blockData.reset();
		if (hasMetadata) {
			blockData.emplace(origin, 0, 0);
			packet >> blockData->inventory >> blockData->meta;
		}
	}

	bool isUpdated = false;
	Network::Command packetType{};
};

#endif // FALLINGBLOCKCOMPONENT_HPP_
