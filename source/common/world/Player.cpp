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
#include "common/network/Network.hpp"
#include "common/world/Player.hpp"

Player::Player() {
	m_hitbox = FloatBox{-0.3f, -0.3f, 0, 0.6f, 0.6f, 1.8f};
}

u16 Player::armorValue() const {
	unsigned protection = 0, maximum = 0, remaining = 0;
	for (const auto &stack : m_armor.items()) {
		if (!stack.amount()) continue;
		u16 durability = stack.maxDamage();
		protection += stack.item().getGroupValue("group:om_armor_points");
		maximum += durability;
		remaining += durability > stack.damage() ? durability - stack.damage() : 0;
	}
	return maximum && protection ? u16((protection - 1) * remaining / maximum + 1) : 0;
}

u16 Player::absorbDamage(u16 amount) {
	unsigned reduced = unsigned(amount) * (25 - std::min<u16>(20, armorValue())) + m_armorRemainder;
	for (u16 slot = 0; slot < 4; ++slot) {
		if (m_armor.getStackRef(0, slot).wear(amount)) m_armor.setChanged(true);
	}
	m_armorRemainder = u16(reduced % 25);
	return u16(reduced / 25);
}

// Note: This function returns an angle4
u8 Player::getDirection() const {
	return int(floorf(m_viewAngleH / 90.f + 0.5f)) & 3;
}

// Note: This function returns an angle4
u8 Player::getOppositeDirection() const {
	return getDirection() ^ 2;
}

Vector3i Player::getCurrentChunk() const {
	return {
		(static_cast<s32>(std::floor(m_x)) & -CHUNK_WIDTH)  / CHUNK_WIDTH,
		(static_cast<s32>(std::floor(m_y)) & -CHUNK_DEPTH)  / CHUNK_DEPTH,
		(static_cast<s32>(std::floor(m_z)) & -CHUNK_HEIGHT) / CHUNK_HEIGHT
	};
}

void Player::serialize(sf::Packet &packet) const {
	packet << m_x << m_y << m_z << m_dimension << m_viewAngleH << m_viewAngleV << m_viewAngleRoll << m_inventory << m_heldItemSlot;
}

void Player::deserialize(sf::Packet &packet) {
	packet >> m_x >> m_y >> m_z >> m_dimension >> m_viewAngleH >> m_viewAngleV >> m_viewAngleRoll >> m_inventory >> m_heldItemSlot;
}

// Please update 'docs/lua-api-cpp.md' if you change this
void Player::initUsertype(sol::state &lua) {
	lua.new_usertype<Player>("Player",
		"name", &Player::name,
		"health", &Player::health,
		"heal", &Player::heal,
		"damage", &Player::damage,

		"x", &Player::x,
		"y", &Player::y,
		"z", &Player::z,
		"set_position", &Player::setPosition,

		"dimension", &Player::dimension,
		"set_dimension", &Player::setDimension,

		"client_id", &Player::clientID,

		"inventory", &Player::inventory,

		"held_item_stack", &Player::heldItemStack
	);
}
