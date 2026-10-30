local modpath = mod:path()

mod:block {
	id = "chest", name = "Chest",
	tiles = {"refi_chest_top.png", "refi_chest_top.png", "refi_chest_front.png", "refi_chest_side.png"},
	is_rotatable = true, hardness = 2.5,
	groups = {om_material_wood = 1, om_fuel = 300},
	on_block_placed = function(pos, world)
		world:add_block_data(pos.x, pos.y, pos.z, 9, 3)
	end,
	on_block_activated = function(pos, block, player, world, client, server)
		local above = world:get_block_state(pos.x, pos.y, pos.z + 1)
		if not above or above:is_opaque() then return end
		local data = world:get_block_data(pos.x, pos.y, pos.z)
		if not data then data = world:add_block_data(pos.x, pos.y, pos.z, 9, 3) end
		world:mark_block_modified(pos.x, pos.y, pos.z)
		local gui = LuaGUI.new()
		gui:set_size(176, 166)
		gui:set_centered(true)
		gui:image {name = "background", pos = {x = 0, y = 0}, texture = modpath .. "/textures/gui/inventory.png",
			size = {x = 176, y = 166}, border = 5}
		gui:inventory {
			name = "chest", pos = {x = 7, y = 17}, size = {x = 9, y = 3},
			inventory = {source = "block", block = {x = pos.x, y = pos.y, z = pos.z}, offset = 0, count = 27},
			shift_destination = "inv_main,inv_hotbar",
		}
		gui:inventory {
			name = "inv_main", pos = {x = 7, y = 83}, size = {x = 9, y = 3},
			inventory = {source = "player", player = "player", inventory_name = "main", offset = 9, count = 27},
			shift_destination = "chest",
		}
		gui:inventory {
			name = "inv_hotbar", pos = {x = 7, y = 141}, size = {x = 9, y = 1},
			inventory = {source = "player", player = "player", inventory_name = "main", offset = 0, count = 9},
			shift_destination = "chest",
		}
		gui:show(client)
	end,
}

mod:crafting_recipe {
	result = {id = "default:chest", amount = 1}, pattern = {"WWW", "W W", "WWW"},
	keys = {W = "group:default_planks"},
}

mod:block {
	id = "mob_spawner", name = "Monster Spawner", tiles = "refi_mob_spawner.png",
	draw_type = "glass", is_opaque = false, hardness = 5,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1},
	item_drop = {id = "default:mob_spawner", amount = 0},
	on_tick = function(pos, block, chunk, world) world:tick_spawner(pos.x, pos.y, pos.z) end,
}

mod:item {id = "cocoa_beans", name = "Cocoa Beans", tiles = "refi_cocoa_beans.png"}

for _, record in ipairs({
	{id = "13", track = 1, title = "The Evil Sister (Jordach's Mix)"},
	{id = "cat", track = 2, title = "The Energetic Rat (Jordach's Mix)"},
}) do
	mod:item {
		id = "record_" .. record.id, name = "Music Disc - " .. record.title,
		tiles = "refi_record_" .. record.id .. ".png", max_stack_size = 1,
		groups = {om_record = record.track},
	}
end

mod:block {
	id = "jukebox", name = "Jukebox", tiles = {"refi_jukebox_top.png", "refi_jukebox_side.png"},
	hardness = 2, groups = {om_material_wood = 1},
	on_block_placed = function(pos, world) world:add_block_data(pos.x, pos.y, pos.z, 1, 1) end,
	on_block_activated = function(pos, block, player, world, client, server)
		world:use_jukebox(pos.x, pos.y, pos.z, player)
	end,
	on_block_destroyed = function(pos, world) world:stop_jukebox(pos.x, pos.y, pos.z) end,
}

mod:crafting_recipe {
	result = {id = "default:jukebox", amount = 1}, pattern = {"WWW", "WDW", "WWW"},
	keys = {W = "group:default_planks", D = "default:diamond"},
}
