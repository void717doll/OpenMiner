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
#ifndef ITEMSTACK_HPP_
#define ITEMSTACK_HPP_

#include "common/core/EngineConfig.hpp"
#include "common/inventory/Item.hpp"

namespace sf { class Packet; }

class ItemStack {
	public:
		ItemStack() = default;
		ItemStack(const std::string &stringID, u16 amount = 1, u16 damage = 0)
			: m_stringID(stringID), m_amount(amount), m_damage(damage) {}

		const Item &item() const;
		const std::string &stringID() const { return m_stringID; }
		void setItem(const std::string &stringID) { m_stringID = stringID; m_damage = 0; }

		u16 amount() const { return m_amount; }
		void setAmount(u16 amount) { m_amount = amount; }
		u16 damage() const { return m_damage; }
		void setDamage(u16 damage) { m_damage = damage; }
		u16 maxDamage() const { return item().getGroupValue("group:om_max_damage"); }
		bool wear(u16 amount);
		bool sameItem(const ItemStack &other) const {
			return m_stringID == other.m_stringID && m_damage == other.m_damage;
		}

		static void initUsertype(sol::state &lua);

		static const ItemStack Empty;

	private:
		std::string m_stringID{BLOCK_AIR};

		u16 m_amount = 0;
		u16 m_damage = 0;
};

sf::Packet &operator<<(sf::Packet &packet, const ItemStack &itemStack);
sf::Packet &operator>>(sf::Packet &packet, ItemStack &itemStack);

#endif // ITEMSTACK_HPP_
