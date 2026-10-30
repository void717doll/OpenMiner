local cookable = {
	["default:porkchop_raw"] = "default:porkchop_cooked",
	["default:fish_cod_raw"] = "default:fish_cod_cooked",
	["default:potato"] = "default:baked_potato",
}

local function campfire_data(pos, world)
	return world:get_block_data(pos.x, pos.y, pos.z) or world:add_block_data(pos.x, pos.y, pos.z, 4, 1)
end

local function show_campfire(pos, client)
	local gui = LuaGUI.new()
	gui:set_size(176, 166)
	gui:set_centered(true)
	gui:image {
		name = "background", pos = {x = 0, y = 0},
		texture = mod:path() .. "/textures/gui/inventory.png", size = {x = 176, y = 166}, border = 5,
	}
	for slot = 0, 3 do
		gui:inventory {
			name = "food_" .. slot, pos = {x = 40 + slot * 26, y = 22},
			inventory = {source = "block", block = pos, offset = slot, count = 1},
			size = {x = 1, y = 1}, is_read_only = true, shift_destination = "inv_main,inv_hotbar",
		}
		gui:progress_bar {
			name = "progress_" .. slot, pos = {x = 37 + slot * 26, y = 45}, type = "item_process",
			block = pos, meta = "cook_" .. slot, max_value = 600,
			texture = mod:path() .. "/textures/gui/furnace.png", clip = {x = 176, y = 14, width = 24, height = 17},
		}
	end
	gui:inventory {
		name = "inv_main", pos = {x = 7, y = 83},
		inventory = {source = "player", player = "player", inventory_name = "main", offset = 9, count = 27},
		size = {x = 9, y = 3}, shift_destination = "inv_hotbar",
	}
	gui:inventory {
		name = "inv_hotbar", pos = {x = 7, y = 141},
		inventory = {source = "player", player = "player", inventory_name = "main", offset = 0, count = 9},
		size = {x = 9, y = 1}, shift_destination = "inv_main",
	}
	gui:show(client)
end

local function quench(pos, world)
	world:set_block_state(pos.x, pos.y, pos.z, 1)
	world:emit_particles(pos.x, pos.y, pos.z, 1)
end

local function activate_campfire(pos, block, player, world, client, server)
	local data = campfire_data(pos, world)
	local state = world:get_block_state(pos.x, pos.y, pos.z)
	local held = player:held_item_stack()
	local id = held:item():string_id()
	if held:amount() > 0 and held:item():has_group("group:om_igniter") then
		if state:id() == 1 then
			world:set_block_state(pos.x, pos.y, pos.z, 0)
			server:wear_held_item(player, 1)
		end
		return
	end
	if held:amount() > 0 and held:item():get_group_value("group:om_tool_kind") == 3 then
		if state:id() == 0 then
			quench(pos, world)
			server:wear_held_item(player, 1)
		end
		return
	end
	if held:amount() > 0 and id == "default:bucket_water" then
		if state:id() == 0 then
			quench(pos, world)
			if not client.creative and server:consume_held_item(player, 1, client.id) then
				mod:give_item_stack(player, ItemStack.new("default:bucket_empty", 1))
			end
		end
		return
	end
	if held:amount() > 0 and cookable[id] then
		for slot = 0, 3 do
			if data.inventory:get_stack(slot, 0):amount() == 0 then
				if not server:consume_held_item(player, 1, client.id) then return end
				data.inventory:set_stack(slot, 0, id, 1)
				data.meta:set_string("input_" .. slot, id)
				data.meta:set_int("cook_" .. slot, 0)
				world:mark_block_modified(pos.x, pos.y, pos.z)
				return
			end
		end
	end
	show_campfire({x = pos.x, y = pos.y, z = pos.z}, client)
end

local function campfire_tick(pos, block, chunk, world)
	local state = world:get_block_state(pos.x, pos.y, pos.z)
	if not state then return end
	local data = campfire_data(pos, world)
	if state:id() == 0 then
		for _, direction in ipairs({{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1}}) do
			local neighbor = world:get_block_state(pos.x + direction[1], pos.y + direction[2], pos.z + direction[3])
			local id = neighbor and neighbor:block():string_id()
			if id == "default:water" or id == "default:water_flowing" then quench(pos, world) return end
		end
	end
	local changed = false
	for slot = 0, 3 do
		local stack = data.inventory:get_stack(slot, 0)
		local id = stack:amount() > 0 and stack:item():string_id() or ""
		if id ~= data.meta:get_string("input_" .. slot) then
			data.meta:set_string("input_" .. slot, id)
			data.meta:set_int("cook_" .. slot, 0)
			changed = true
		end
		if state:id() == 0 and stack:amount() == 1 and cookable[id] then
			local progress = (data.meta:get_int("cook_" .. slot) or 0) + 1
			data.meta:set_int("cook_" .. slot, progress)
			if progress >= 600 then
				data.inventory:set_stack(slot, 0, cookable[id], 1)
				data.meta:set_string("input_" .. slot, cookable[id])
			end
			changed = true
		end
	end
	if changed then world:mark_block_modified(pos.x, pos.y, pos.z) end
end

mod:block {
	id = "campfire", name = "Campfire", tiles = "refi_campfire_log.png",
	inventory_image = "refi_campfire.png", draw_type = "campfire",
	bounding_box = {0, 0, 0, 1, 1, 7 / 16}, is_opaque = false, is_light_source = true,
	hardness = 2, groups = {om_material_wood = 1, attached_node = 3, om_light_level = 15},
	states = {{is_light_source = false, inventory_image = "refi_campfire_unlit.png"}},
	item_drop = {id = "default:charcoal", amount = 2},
	on_block_placed = function(pos, world) world:add_block_data(pos.x, pos.y, pos.z, 4, 1) end,
	on_block_activated = activate_campfire, on_tick = campfire_tick,
}

mod:crafting_recipe {
	result = {id = "default:campfire", amount = 1}, pattern = {" S ", "SCS", "LLL"},
	keys = {S = "default:stick", C = "default:coal", L = "default:oak_wood"},
}
mod:crafting_recipe {
	result = {id = "default:campfire", amount = 1}, pattern = {" S ", "SCS", "LLL"},
	keys = {S = "default:stick", C = "default:charcoal", L = "default:oak_wood"},
}
