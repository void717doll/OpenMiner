#ifndef PLAYERSTATISTICS_HPP_
#define PLAYERSTATISTICS_HPP_

#include <array>
#include <map>
#include <stdexcept>
#include <string>
#include <SFML/Network/Packet.hpp>
#include "common/core/IntTypes.hpp"

struct ItemStatistics {
	u64 mined = 0, placed = 0, crafted = 0, used = 0, broken = 0;
};

struct PlayerStatistics {
	enum General { PlayTicks, WalkCm, SwimCm, FallCm, ClimbCm, FlyCm, CartCm,
		Jumps, Dropped, DamageDealt, DamageTaken, Deaths, MobKills, Count };
	u64 revision = 0;
	std::array<u64, Count> general{};
	std::map<std::string, ItemStatistics> items;

	void write(sf::Packet &packet) const {
		packet << u8(1);
		for (auto value : general) packet << sf::Uint64(value);
		packet << u32(items.size());
		for (const auto &entry : items) {
			const auto &item = entry.second;
			packet << entry.first << sf::Uint64(item.mined) << sf::Uint64(item.placed)
				<< sf::Uint64(item.crafted) << sf::Uint64(item.used) << sf::Uint64(item.broken);
		}
	}

	void read(sf::Packet &packet) {
		u8 version = 0;
		if (!(packet >> version) || version != 1) throw std::runtime_error("unsupported statistics version");
		PlayerStatistics result;
		for (auto &value : result.general) {
			sf::Uint64 number = 0;
			packet >> number;
			value = number;
		}
		u32 count = 0;
		if (!(packet >> count) || count > 10000) throw std::runtime_error("invalid statistics item count");
		for (u32 i = 0; i < count; ++i) {
			std::string name;
			sf::Uint64 mined = 0, placed = 0, crafted = 0, used = 0, broken = 0;
			if (!(packet >> name >> mined >> placed >> crafted >> used >> broken))
				throw std::runtime_error("incomplete statistics");
			result.items[name] = {mined, placed, crafted, used, broken};
		}
		result.revision = revision + 1;
		*this = std::move(result);
	}
};

#endif
