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
#include "common/core/Debug.hpp"
#include "common/core/EngineConfig.hpp"
#include "common/core/Registry.hpp"
#include "common/inventory/Inventory.hpp"
#include "common/network/Network.hpp"

void Inventory::setStack(u16 x, u16 y, const std::string &stringID, u16 amount) {
	m_items.at(x + y * m_width) = ItemStack(stringID, amount);
	m_hasChanged = true;
}

void Inventory::setItemStack(u16 x, u16 y, const ItemStack &stack) {
	m_items.at(x + y * m_width) = stack;
	m_hasChanged = true;
}

ItemStack Inventory::addStack(const std::string &stringID, u16 amount, u16 offset, u16 size, bool mergeOnly, bool retryAfterMerge) {
	return addItemStack(ItemStack{stringID, amount}, offset, size, mergeOnly, retryAfterMerge);
}

ItemStack Inventory::addItemStack(const ItemStack &stack, u16 offset, u16 size, bool mergeOnly, bool retryAfterMerge) {
	if (stack.item().id() == 0 || stack.amount() == 0) return ItemStack::Empty;
	ItemStack remaining = stack;
	std::size_t end = size ? std::min(m_items.size(), std::size_t(offset) + size) : m_items.size();
	for (std::size_t i = offset; remaining.amount() && i < end; ++i) {
		ItemStack &target = m_items[i];
		bool empty = target.amount() == 0 || target.item().id() == 0;
		if ((empty && mergeOnly) || (!empty && !target.sameItem(stack))) continue;
		unsigned maximum = stack.item().maxStackSize();
		unsigned count = empty ? 0 : target.amount();
		if (count >= maximum) continue;
		u16 transferred = u16(std::min(unsigned(remaining.amount()), maximum - count));
		target = ItemStack{stack.item().stringID(), u16(count + transferred), stack.damage()};
		remaining.setAmount(remaining.amount() - transferred);
		m_hasChanged = true;
	}
	if (mergeOnly && retryAfterMerge && remaining.amount())
		return addItemStack(remaining, offset, size);
	return remaining;
}

// NOTE: This fonction is only used by Lua since default parameters don't work properly
ItemStack Inventory::addStack2(const std::string &stringID, u16 amount) {
	return addStack(stringID, amount, 0, 0);
}

void Inventory::clearStack(u16 x, u16 y) {
	setStack(x, y, BLOCK_AIR, 0);
}

void Inventory::serialize(sf::Packet &packet) const {
	packet << m_width << m_height << m_name << u8(m_inBlock)
		<< s32(m_blockPos.x) << s32(m_blockPos.y) << s32(m_blockPos.z)
		<< false;

	packet << u16(m_items.size());

	int i = 0;
	for (auto &it : m_items) {
		packet << it
			<< u8(i % m_width) << u8(i / m_width);
		++i;
	}
}

void Inventory::deserialize(sf::Packet &packet) {
	u8 inBlock;
	bool legacyUnlimited;
	s32 bx, by, bz;
	packet >> m_width >> m_height >> m_name >> inBlock >> bx >> by >> bz >> legacyUnlimited;

	m_inBlock = inBlock;
	m_blockPos = Vector3i{bx, by, bz};

	if (m_items.size() != m_width * m_height)
		m_items.resize(m_width * m_height);

	u16 itemListSize, i = 0;
	packet >> itemListSize;

	while (i < itemListSize && packet) {
		ItemStack stack;
		u8 x = 0, y = 0;
		packet >> stack >> x >> y;
		if (!packet) break;
		if (x < m_width && y < m_height && Registry::getInstance().hasItem(stack.stringID()))
			setItemStack(x, y, stack);
		++i;
	}
}

// Please update 'docs/lua-api-cpp.md' if you change this
void Inventory::initUsertype(sol::state &lua) {
	lua.new_usertype<Inventory>("Inventory",
		"add_stack", sol::overload(&Inventory::addStack, &Inventory::addStack2),
		"get_stack", &Inventory::getStack,
		"set_stack", &Inventory::setStack
	);
}

