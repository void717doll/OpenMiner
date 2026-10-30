farming = {crops = {["default:seeds"] = 7}}

local function loaded_state(world, x, y, z)
	if world:get_light(x, y, z) < 0 then return nil end
	return world:get_block_state(x, y, z)
end

function farming.grow(pos, world, stages)
	local state = loaded_state(world, pos.x, pos.y, pos.z)
	if not state then return false end
	local maximum = farming.crops[state:block():string_id()]
	if not maximum or state:id() >= maximum or stages < 1 then return false end
	local soil = loaded_state(world, pos.x, pos.y, pos.z - 1)
	if not soil or soil:block():string_id() ~= "default:farmland" then return false end
	if world:get_light(pos.x, pos.y, pos.z + 1) < 9 then return false end
	world:set_block_state(pos.x, pos.y, pos.z, math.min(maximum, state:id() + stages))
	return true
end

function farming.grow_tick(pos, block, chunk, world)
	local state = loaded_state(world, pos.x, pos.y, pos.z)
	if not state or state:id() >= 7 then return end
	if world:get_light(pos.x, pos.y, pos.z + 1) < 9 then return end
	local id = state:block():string_id()
	local rate = 1
	local along_x, along_y, diagonal = false, false, false
	for dx = -1, 1 do
		for dy = -1, 1 do
			local soil = loaded_state(world, pos.x + dx, pos.y + dy, pos.z - 1)
			if not soil then return end
			if soil:block():string_id() == "default:farmland" then
				local value = soil:id() > 0 and 3 or 1
				if dx ~= 0 or dy ~= 0 then value = value / 4 end
				rate = rate + value
			end
			local neighbor = loaded_state(world, pos.x + dx, pos.y + dy, pos.z)
			if not neighbor then return end
			if neighbor:block():string_id() == id then
				if dx ~= 0 and dy ~= 0 then diagonal = true
				elseif dx ~= 0 then along_x = true
				elseif dy ~= 0 then along_y = true end
			end
		end
	end
	-- beta crop rows grow faster than a crowded square of the same crop.
	if diagonal or (along_x and along_y) then rate = rate / 2 end
	if math.random(math.floor(100 / rate)) == 1 then farming.grow(pos, world, 1) end
end

function farming.soil_tick(pos, block, chunk, world)
	local soil = loaded_state(world, pos.x, pos.y, pos.z)
	local above = loaded_state(world, pos.x, pos.y, pos.z + 1)
	if not soil or not above or soil:block():string_id() ~= "default:farmland" then return end
	if above:block():id() ~= 0 and above:is_collidable() then
		world:set_block_from_str(pos.x, pos.y, pos.z, "default:dirt")
		return
	end
	local wet, unloaded = false, false
	for dx = -4, 4 do
		for dy = -4, 4 do
			for dz = 0, 1 do
				local neighbor = loaded_state(world, pos.x + dx, pos.y + dy, pos.z + dz)
				if not neighbor then unloaded = true
				else
					local id = neighbor:block():string_id()
					if id == "default:water" or id == "default:water_flowing" then wet = true end
				end
			end
		end
	end
	if wet then
		if soil:id() ~= 7 then world:set_block_state(pos.x, pos.y, pos.z, 7) end
	elseif not unloaded then
		if soil:id() > 0 then world:set_block_state(pos.x, pos.y, pos.z, soil:id() - 1)
		elseif not farming.crops[above:block():string_id()] then
			world:set_block_from_str(pos.x, pos.y, pos.z, "default:dirt")
		end
	end
end

local function register_crop(def)
	local id = "default:" .. def.seed
	farming.crops[id] = 7
	local states = {}
	for age = 1, 7 do
		local texture = math.floor(age * (#def.stages - 1) / 7) + 1
		states[age] = {
			tiles = def.stages[texture],
			bounding_box = {0, 0, 0, 1, 1, (age + 1) / 8},
		}
	end
	states[7].item_drop = {id = "default:" .. def.produce, amount = 1}
	mod:block {
		id = def.seed,
		name = def.name,
		tiles = def.stages[1],
		inventory_image = def.icon,
		draw_type = "xshape",
		hardness = 0,
		groups = {om_crop = 7, attached_node = 3},
		item_drop = {id = id, amount = 1},
		bounding_box = {0, 0, 0, 1, 1, 1 / 8},
		draw_offset = {0, 0, -1 / 16},
		placement_constraints = {
			[{0, 0, -1}] = {block = "default:farmland", is_whitelist = true},
		},
		states = states,
		tick_randomly = true,
		tick_probability = 0.01,
		on_tick = farming.grow_tick,
	}
	mod:block {
		id = "wild_" .. def.produce,
		name = "Wild " .. def.crop_name,
		tiles = def.stages[#def.stages],
		draw_type = "xshape",
		hardness = 0,
		groups = {attached_node = 3},
		item_drop = {id = id, amount = 1},
		placement_constraints = {
			[{0, 0, -1}] = {block = "group:om_material_dirt", is_whitelist = true},
		},
	}
end

local function stage_textures(prefix, count)
	local result = {}
	for stage = 1, count do result[stage] = prefix .. stage .. ".png" end
	return result
end

register_crop {
	seed = "potato_eyes", name = "Potato Eyes", crop_name = "Potato", produce = "potato",
	icon = "refi_crops_potato_eyes.png", stages = stage_textures("refi_crops_potato_plant_", 5),
}
register_crop {
	seed = "tomato_seeds", name = "Tomato Seeds", crop_name = "Tomato", produce = "tomato",
	icon = "refi_crops_tomato_seed.png", stages = stage_textures("refi_crops_tomato_plant_", 6),
}
register_crop {
	seed = "barley_seeds", name = "Barley Seeds", crop_name = "Barley", produce = "barley",
	icon = "refi_farming_barley_seed.png", stages = stage_textures("refi_farming_barley_", 7),
}

mod:item {id = "potato", name = "Potato", tiles = "refi_crops_potato.png", heal_amount = 1}
mod:item {id = "tomato", name = "Tomato", tiles = "refi_crops_tomato.png", heal_amount = 2}
mod:item {id = "barley", name = "Barley", tiles = "refi_farming_barley.png"}
mod:item {id = "baked_potato", name = "Baked Potato", tiles = "refi_farming_baked_potato.png", heal_amount = 5}
mod:item {id = "barley_bread", name = "Barley Bread", tiles = "refi_farming_bread_multigrain.png", heal_amount = 5}
mod:item {
	id = "vegetable_stew", name = "Vegetable Stew", tiles = "refi_crops_bowl_vegetable_stew.png",
	heal_amount = 7, container_item = "default:bowl",
}

mod:smelting_recipe {
	input = {id = "default:potato", amount = 1},
	output = {id = "default:baked_potato", amount = 1},
}
mod:crafting_recipe {
	result = {id = "default:barley_bread", amount = 1}, pattern = {"BBB"},
	keys = {B = "default:barley"},
}
mod:crafting_recipe {
	result = {id = "default:vegetable_stew", amount = 1}, pattern = {"PT", "B "},
	keys = {P = "default:potato", T = "default:tomato", B = "default:bowl"},
}
for _, recipe in ipairs({{"potato", "potato_eyes"}, {"tomato", "tomato_seeds"}, {"barley", "barley_seeds"}}) do
	mod:crafting_recipe {
		result = {id = "default:" .. recipe[2], amount = 2}, pattern = {"P"},
		keys = {P = "default:" .. recipe[1]},
	}
end
