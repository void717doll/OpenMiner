#ifndef BETAMINING_HPP_
#define BETAMINING_HPP_

#include <cmath>
#include <limits>

#include "common/inventory/Item.hpp"
#include "common/world/Block.hpp"

namespace retro {

inline u16 attackDamage(const Item &item) {
	u16 damage = item.getGroupValue("group:om_attack_damage");
	return damage ? damage : 1;
}

inline bool canHarvest(const BlockState &state, const Item &item) {
	if (!state.block().id() || !std::isfinite(state.hardness()) || state.hardness() < 0
	 || state.drawType() == BlockDrawType::Liquid) return false;
	if (!(state.harvestRequirements() & 1)) return true;
	if (!(item.harvestCapability() & 1)) return false;

	u16 level = item.getGroupValue("group:om_harvest_level");
	const std::string &id = state.block().stringID();
	if (id == "default:obsidian") return level == 3;
	if (id == "default:diamond_ore" || id == "default:diamond_block"
	 || id == "default:gold_ore" || id == "default:gold_block"
	 || id == "default:redstone_ore" || id == "default:redstone_block") return level >= 2;
	if (id == "default:iron_ore" || id == "default:iron_block"
	 || id == "default:lapis_ore" || id == "default:lapis_block") return level >= 1;
	return true;
}

inline float breakTime(const BlockState &state, const Item &item) {
	if (!state.block().id() || !std::isfinite(state.hardness()) || state.hardness() < 0
	 || state.drawType() == BlockDrawType::Liquid) return std::numeric_limits<float>::infinity();
	if (!canHarvest(state, item)) return 5.f * state.hardness();

	bool effective = (state.harvestRequirements() & item.harvestCapability()) != 0;
	for (const auto &target : item.effectiveOn()) {
		if (target == state.block().stringID() || state.block().hasGroup(target)) {
			effective = true;
			break;
		}
	}
	// beta's pickaxe effective-block list omits obsidian and redstone ore.
	if (state.block().stringID() == "default:obsidian" || state.block().stringID() == "default:redstone_ore")
		effective = false;
	float speed = effective ? item.miningSpeed() : 1.f;
	if (!std::isfinite(speed) || speed <= 0) speed = 1.f;
	return 1.5f * state.hardness() / speed;
}

}

#endif // BETAMINING_HPP_
