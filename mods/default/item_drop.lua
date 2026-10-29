--
-- =====================================================================================
--
--  OpenMiner
--
--  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
--  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
--
--  This file is part of OpenMiner.
--
--  OpenMiner is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public
--  License as published by the Free Software Foundation; either
--  version 2.1 of the License, or (at your option) any later version.
--
--  OpenMiner is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public License
--  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
--  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
--
-- =====================================================================================
--

mod:entity {
	id = "item_drop",

	properties = {
		visual = {
			type = "inventorycube",
			size = 0.25,
			origin = {0.125, 0.125, 0.125},
		},

		is_rotatable = true,

		animation = {
			{
				type = "rotation",
				axis = {0, 0, 1},
				angle = 0.5
			},
		},

		hitbox = {0, 0, 0, 0.25, 0.25, 0.25},
	},

	on_collision = function(entity, player, server)
		local stack = entity:item_stack()
		local remainder = mods["default"]:give_item_stack(player, stack)
		if remainder:amount() == 0 then
			mods["default"]:despawn_entity(entity)
		else
			stack:set_amount(remainder:amount())
		end
	end,
}

openminer:add_listener(Event.BlockDigged, function(pos, block, player, world, client, server)
	local held_item = player:held_item_stack():item()
	if not block:can_harvest(held_item) then return end
	local drop = block:get_item_drop()
	local id = block:block():string_id()
	if held_item:string_id() == "default:shears" and (id == "default:oak_leaves" or id == "default:tallgrass") then
		drop = ItemStack.new(id, 1)
	elseif id == "default:diamond_ore" then
		drop = ItemStack.new("default:diamond", 1)
	elseif id == "default:redstone_ore" then
		drop = ItemStack.new("default:redstone_dust", math.random(4, 5))
	elseif id == "default:lapis_ore" then
		drop = ItemStack.new("default:lapis_lazuli", math.random(4, 8))
	elseif id == "default:tallgrass" then
		if math.random(8) ~= 1 then return end
		drop = ItemStack.new("default:seeds", 1)
	elseif id == "default:oak_leaves" then
		if math.random(20) ~= 1 then return end
		local registered = pcall(function()
			return openminer.registry:get_item_from_string("default:sapling")
		end)
		if not registered then return end
		drop = ItemStack.new("default:sapling", 1)
	end
	if drop:amount() == 0 or drop:item():id() == 0 then return end
	mods["default"]:spawn_entity("default:item_drop", {
		position = {pos.x + 0.5, pos.y + 0.5, pos.z + 0.5},
		dimension = world:dimension():id(),
		item_stack = {drop:item():string_id(), drop:amount()},
	})
end)

