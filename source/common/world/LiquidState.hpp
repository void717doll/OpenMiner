#ifndef LIQUIDSTATE_HPP_
#define LIQUIDSTATE_HPP_

#include "common/world/Block.hpp"

namespace liquid {
inline int kind(const Block &block) {
	if (block.getGroupValue("group:om_waterlogged")) return 1;
	const auto &name = block.stringID();
	if (name == "default:water" || name == "default:water_flowing") return 1;
	if (name == "default:lava" || name == "default:lava_flowing") return 2;
	return 0;
}
inline float height(const Block &block, u16 data, bool covered = false) {
	if (covered || block.getGroupValue("group:om_waterlogged")) return 1;
	const auto &name = block.stringID();
	if (name == "default:water_flowing" || name == "default:lava_flowing") {
		u16 param = block.param().getParam(BlockParam::Custom, data);
		if (param & 8) return 1;
		return float((param & 7) + 1) / 8 * (14.f / 16.f);
	}
	return 14.f / 16.f;
}
}

#endif // LIQUIDSTATE_HPP_
