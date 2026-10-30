local flowers = {["default:rose"] = true, ["default:dandelion"] = true}
local hives = {["default:wild_hive"] = true, ["default:beehive"] = true}
local check_interval = 200
local honey_interval = 4800
local pollination_interval = 2400

local function metadata(pos, world)
	local data = world:get_block_data(pos.x, pos.y, pos.z)
	if not data then data = world:add_block_data(pos.x, pos.y, pos.z, 0, 0) end
	return data.meta
end

local function timestamp(meta, name)
	return tonumber(meta:get_string(name)) or 0
end

local function surroundings(pos, world)
	local flower_count, crops = 0, {}
	for z = pos.z - 2, pos.z + 2 do
		for y = pos.y - 4, pos.y + 4 do
			for x = pos.x - 4, pos.x + 4 do
				if world:get_light(x, y, z) >= 0 then
					local state = world:get_block_state(x, y, z)
					if state then
						local id = state:block():string_id()
						if flowers[id] then flower_count = flower_count + 1 end
						local maximum = farming.crops[id]
						if maximum and state:id() < maximum then
							crops[#crops + 1] = {x = x, y = y, z = z}
						end
					end
				end
			end
		end
	end
	return flower_count, crops
end

local function hive_tick(pos, block, chunk, world)
	local now = world:get_ticks()
	local meta = metadata(pos, world)
	local previous = timestamp(meta, "bee_check")
	if now >= previous and now - previous < check_interval then return end
	meta:set_string("bee_check", tostring(now))
	meta:set_int("bee_active", 0)
	if world:dimension():id() ~= 0 or not world:is_daytime()
		or world:get_light(pos.x, pos.y, pos.z + 1) < 12 then
		world:mark_block_modified(pos.x, pos.y, pos.z)
		return
	end
	local flower_count, crops = surroundings(pos, world)
	if flower_count < 2 then
		world:mark_block_modified(pos.x, pos.y, pos.z)
		return
	end
	meta:set_int("bee_active", 1)
	local honey = meta:get_int("bee_honey") or 0
	if honey < 3 then
		-- only count a loaded check, never time spent in an unloaded chunk.
		local progress = (meta:get_int("bee_progress") or 0) + check_interval
		if progress >= honey_interval then
			meta:set_int("bee_honey", honey + 1)
			progress = progress - honey_interval
		end
		meta:set_int("bee_progress", progress)
	end
	local last_pollination = timestamp(meta, "bee_pollination")
	if now >= last_pollination and now - last_pollination < pollination_interval then
		world:mark_block_modified(pos.x, pos.y, pos.z)
		return
	end
	meta:set_string("bee_pollination", tostring(now))
	world:mark_block_modified(pos.x, pos.y, pos.z)
	if #crops == 0 then return end
	local first = math.random(#crops)
	for offset = 0, #crops - 1 do
		local crop = crops[(first + offset - 1) % #crops + 1]
		local data = world:get_block_data(crop.x, crop.y, crop.z)
		local last = data and timestamp(data.meta, "bee_pollination") or 0
		-- a shared crop cooldown prevents overlapping hives multiplying growth.
		if (last == 0 or now < last or now - last >= pollination_interval)
			and farming.grow(crop, world, 1) then
			metadata(crop, world):set_string("bee_pollination", tostring(now))
			world:mark_block_modified(crop.x, crop.y, crop.z)
			break
		end
	end
end

local function smoke_hive(pos, block, player, world, client, server)
	if not hives[block:string_id()] then return false end
	local meta = metadata(pos, world)
	local now = world:get_ticks()
	if timestamp(meta, "bee_smoke") > now then return true end
	meta:set_string("bee_smoke", tostring(now + 900))
	world:mark_block_modified(pos.x, pos.y, pos.z)
	server:wear_held_item(player, 1)
	world:emit_particles(pos.x, pos.y, pos.z, 1)
	return true
end

local function campfire_smoke(pos, world)
	for distance = 1, 5 do
		if world:get_light(pos.x, pos.y, pos.z - distance) < 0 then return false end
		local state = world:get_block_state(pos.x, pos.y, pos.z - distance)
		if not state then return false end
		if state:block():string_id() == "default:campfire" then return state:id() == 0 end
		if state:block():id() ~= 0 and state:is_collidable() then return false end
	end
	return false
end

local function harvest(pos, block, player, world, client, server)
	local held = player:held_item_stack()
	if held:amount() > 0 and held:item():string_id() == "default:bee_smoker" then
		smoke_hive(pos, block, player, world, client, server)
		return
	end
	local meta = metadata(pos, world)
	local honey = meta:get_int("bee_honey") or 0
	if honey == 0 then return end
	local remainder = mod:give_item_stack(player, ItemStack.new("default:honeycomb", 1))
	if remainder:amount() ~= 0 then return end
	meta:set_int("bee_honey", honey - 1)
	world:mark_block_modified(pos.x, pos.y, pos.z)
	local now = world:get_ticks()
	if not client.creative and timestamp(meta, "bee_smoke") <= now
		and timestamp(meta, "bee_sting") <= now and not campfire_smoke(pos, world) then
		meta:set_string("bee_sting", tostring(now + 40))
		server:damage_player(client.id, 2, "bees")
	end
end

for _, hive in ipairs({
	{id = "wild_hive", name = "Wild Beehive", tiles = "refi_bees_wild_hive.png", drop = "default:honeycomb"},
	{id = "beehive", name = "Beehive", tiles = {"refi_bees_top.png", "refi_bees_top.png",
		"refi_bees_hive_artificial.png", "refi_bees_side.png"}, drop = "default:beehive"},
}) do
	mod:block {
		id = hive.id, name = hive.name, tiles = hive.tiles,
		hardness = 1, groups = {attached_node = 3, om_material_wood = 1, om_fuel = 300},
		item_drop = {id = hive.drop, amount = 1},
		tick_randomly = true, tick_probability = 0.05,
		on_tick = hive_tick, on_block_activated = harvest,
	}
end

mod:item {id = "honeycomb", name = "Honeycomb", tiles = "refi_bees_comb.png"}
mod:item {id = "honey", name = "Honey", tiles = "refi_bees_bottle_honey.png", heal_amount = 4, max_stack_size = 1}
mod:item {id = "beeswax", name = "Beeswax", tiles = "refi_bees_wax.png", groups = {om_fuel = 200}}
mod:item {
	id = "bee_smoker", name = "Bee Smoker", tiles = "refi_bees_smoker.png", max_stack_size = 1,
	groups = {om_max_damage = 63}, on_item_activated = smoke_hive,
}
mod:crafting_recipe {
	result = {id = "default:beehive", amount = 1}, pattern = {"PPP", "PCP", "PPP"},
	keys = {P = "default:oak_planks", C = "default:honeycomb"},
}
mod:crafting_recipe {
	result = {id = "default:bee_smoker", amount = 1}, pattern = {" I ", "ICI", " I "},
	keys = {I = "default:iron_ingot", C = "default:coal"},
}
mod:crafting_recipe {
	result = {id = "default:honey", amount = 1}, pattern = {"C"}, keys = {C = "default:honeycomb"},
}
mod:crafting_recipe {
	result = {id = "default:beeswax", amount = 2}, pattern = {"CC"}, keys = {C = "default:honeycomb"},
}
mod:crafting_recipe {
	result = {id = "default:torch", amount = 4}, pattern = {"W", "S"},
	keys = {W = "default:beeswax", S = "default:stick"},
}
