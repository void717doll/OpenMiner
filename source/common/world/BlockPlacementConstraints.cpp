/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#include "common/core/Registry.hpp"
#include "common/world/BlockPlacementConstraints.hpp"
#include "common/world/World.hpp"

bool BlockPlacementConstraints::check(const World &world, const Vector3i &pos) const {
	for (const auto &constraint : m_constraints) {
		const BlockState *state = world.getBlockState(
			pos.x + constraint.blockOffset.x,
			pos.y + constraint.blockOffset.y,
			pos.z + constraint.blockOffset.z);
		// unloaded terrain cannot satisfy placement constraints.
		if (!state)
			return false;

		const Block &block = state->block();
		bool matches = constraint.blockID.compare(0, 6, "group:") == 0
			? block.getGroupValue(constraint.blockID) > 0
			: block.stringID() == constraint.blockID;
		if (constraint.isWhitelist != matches)
			return false;
	}

	return true;
}

void BlockPlacementConstraints::serialize(sf::Packet &packet) const {
	packet << m_constraints;
}

void BlockPlacementConstraints::deserialize(sf::Packet &packet) {
	packet >> m_constraints;
}

