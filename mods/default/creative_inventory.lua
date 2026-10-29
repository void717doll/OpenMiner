function show_creative_inventory(key_id, client)
	local gui = LuaGUI.new(key_id)
	gui:set_size(176, 238)
	gui:set_centered(true)
	gui:image {
		name = "img_background", pos = {x = 0, y = 0},
		texture = mod:path() .. "/textures/gui/inventory.png",
		size = {x = 176, y = 238}, border = 5,
	}
	gui:text_input {
		name = "search", pos = {x = 7, y = 7}, width = 162, height = 14,
		placeholder = "search items", inventory = "inv_catalog",
	}
	gui:inventory {
		name = "inv_catalog", pos = {x = 7, y = 25},
		inventory = {source = "catalog", count = 45}, size = {x = 9, y = 5},
		is_read_only = true, shift_destination = "inv_hotbar,inv_main",
	}
	gui:inventory {
		name = "inv_main", pos = {x = 7, y = 121},
		inventory = {source = "player", player = "player", inventory_name = "main", offset = 9, count = 27},
		size = {x = 9, y = 3}, shift_destination = "inv_hotbar",
	}
	gui:inventory {
		name = "inv_hotbar", pos = {x = 7, y = 187},
		inventory = {source = "player", player = "player", inventory_name = "main", offset = 0, count = 9},
		size = {x = 9, y = 1}, shift_destination = "inv_main",
	}
	for slot, piece in ipairs({"helmet", "chestplate", "leggings", "boots"}) do
		gui:inventory {
			name = "armor_" .. piece, pos = {x = 7 + (slot - 1) * 18, y = 211},
			inventory = {source = "player", player = "player", inventory_name = "armor", offset = slot - 1, count = 1},
			size = {x = 1, y = 1}, filter = "group:om_armor_" .. piece,
			shift_destination = "inv_main,inv_hotbar",
		}
	end
	gui:show(client)
end
