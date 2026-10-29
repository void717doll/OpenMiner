#ifndef BETATOOLRULES_HPP_
#define BETATOOLRULES_HPP_

#include "common/inventory/ItemStack.hpp"
#include "common/world/BetaMining.hpp"

namespace retro {

inline u16 diggingWear(const Item &item, const BlockState &state) {
	u16 type = item.getGroupValue("group:om_tool_type");
	if (type == 1) return 2;
	if (type == 2) return 1;
	if (type == 4 && (state.drawType() == BlockDrawType::Leaves || state.block().stringID() == "default:web")) return 1;
	return 0;
}

inline u16 attackingWear(const Item &item) {
	u16 type = item.getGroupValue("group:om_tool_type");
	return type == 1 ? 1 : type == 2 ? 2 : 0;
}

inline float survivalBreakTime(const BlockState &state, const Item &item, bool submerged, bool grounded) {
	float time = breakTime(state, item);
	if (!std::isfinite(time) || !canHarvest(state, item)) return time;
	u16 type = item.getGroupValue("group:om_tool_type");
	u16 kind = item.getGroupValue("group:om_tool_kind");
	const std::string &id = state.block().stringID();
	if (kind) {
		bool effective = false;
		if (kind == 1) effective = id == "default:oak_planks" || id == "default:oak_wood"
			|| id == "default:bookshelf" || id == "default:chest";
		if (kind == 2) {
			for (const char *block : {"cobblestone", "cobblestone_mossy", "stone", "sandstone", "iron_ore",
				"iron_block", "coal_ore", "gold_block", "gold_ore", "diamond_ore", "diamond_block", "ice",
				"netherrack", "lapis_ore", "lapis_block", "stone_slab", "double_stone_slab"})
				if (id == std::string("default:") + block) { effective = true; break; }
		}
		if (kind == 3) {
			for (const char *block : {"grass", "dirt", "sand", "gravel", "snow", "snow_layer", "clay", "farmland"})
				if (id == std::string("default:") + block) { effective = true; break; }
		}
		time = 1.5f * state.hardness() / (effective ? item.miningSpeed() : 1.f);
	}
	if (type == 1) time = 1.5f * state.hardness() / (state.block().stringID() == "default:web" ? 15.f : 1.5f);
	if (type == 4) {
		float speed = state.drawType() == BlockDrawType::Leaves || state.block().stringID() == "default:web" ? 15.f
			: state.block().stringID() == "default:wool" ? 5.f : 1.f;
		time = 1.5f * state.hardness() / speed;
	}
	if (submerged) time *= 5;
	if (!grounded) time *= 5;
	return time;
}

}

#endif // BETATOOLRULES_HPP_
