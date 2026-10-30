local blocks, items = {}, {}
mod = {
	block = function(_, def) blocks["default:" .. def.id] = def end,
	item = function(_, def) items["default:" .. def.id] = def end,
	crafting_recipe = function() end,
	smelting_recipe = function() end,
}
dofile("mods/default/farming.lua")
dofile("mods/default/bonemeal.lua")

local cells, light, unloaded = {}, 15, false
local function key(x, y, z) return x .. "," .. y .. "," .. z end
local function place(x, y, z, id, age, solid)
	local block = {string_id = function() return id end, id = function() return id == "air" and 0 or 1 end}
	local state = {age = age or 0, block = function() return block end, is_collidable = function() return solid or false end}
	state.id = function(self) return self.age end
	cells[key(x, y, z)] = state
	return state
end
local air = place(-99, 0, 0, "air")
local world = {
	emit_particles = function() end,
	get_light = function(_, x, y, z) if unloaded and x >= 4 then return -1 end return light end,
	get_block_state = function(_, x, y, z) return cells[key(x, y, z)] or air end,
	set_block_state = function(_, x, y, z, age) cells[key(x, y, z)].age = age end,
	set_block_from_str = function(_, x, y, z, id) place(x, y, z, id) end,
}
local pos = {x = 0, y = 0, z = 1}
local soil_pos = {x = 0, y = 0, z = 0}
local soil = place(0, 0, 0, "default:farmland", 0)
local plant = place(0, 0, 1, "default:tomato_seeds", 0)
light = 8
assert(not farming.grow(pos, world, 1) and plant.age == 0)
light = 9
assert(farming.grow(pos, world, 1) and plant.age == 1)
place(0, 0, 0, "default:dirt")
assert(not farming.grow(pos, world, 1))
soil = place(0, 0, 0, "default:farmland", 0)
place(4, 4, 1, "default:water_flowing")
farming.soil_tick(soil_pos, nil, nil, world)
assert(soil.age == 7)
cells[key(4, 4, 1)] = nil
unloaded = true
farming.soil_tick(soil_pos, nil, nil, world)
assert(soil.age == 7)
unloaded = false
place(5, 0, 0, "default:water")
farming.soil_tick(soil_pos, nil, nil, world)
assert(soil.age == 6)
soil.age = 0
farming.soil_tick(soil_pos, nil, nil, world)
assert(soil:block():string_id() == "default:farmland")
cells[key(0, 0, 1)] = nil
farming.soil_tick(soil_pos, nil, nil, world)
assert(world:get_block_state(0, 0, 0):block():string_id() == "default:dirt")
soil = place(0, 0, 0, "default:farmland", 7)
place(0, 0, 1, "default:stone", 0, true)
farming.soil_tick(soil_pos, nil, nil, world)
assert(world:get_block_state(0, 0, 0):block():string_id() == "default:dirt")
soil = place(0, 0, 0, "default:farmland", 7)
plant = place(0, 0, 1, "default:potato_eyes", 0)
local remaining, consumed = 2, 0
local stack = {amount = function() return remaining end}
local player = {held_item_stack = function() return stack end}
local server = {consume_held_item = function(_, _, amount) remaining = remaining - amount consumed = consumed + amount end}
farming.use_fertilizer(pos, nil, player, world, {id = 1}, server, 2)
assert(plant.age == 3 and remaining == 1 and consumed == 1)
farming.use_fertilizer(soil_pos, nil, player, world, {id = 1}, server, 3)
assert(plant.age == 7 and remaining == 0 and consumed == 2)
remaining = 1
farming.use_fertilizer(pos, nil, player, world, {id = 1}, server, 2)
assert(remaining == 1 and consumed == 2)
light = 0
plant.age = 0
farming.use_fertilizer(pos, nil, player, world, {id = 1}, server, 2)
assert(remaining == 1 and plant.age == 0)
light = 15
pos.x = 4
plant = place(4, 0, 1, "default:barley_seeds", 0)
place(4, 0, 0, "default:farmland", 7)
unloaded = true
assert(not farming.grow(pos, world, 1) and plant.age == 0)
for _, name in ipairs({"potato_eyes", "tomato_seeds", "barley_seeds"}) do
	local def = blocks["default:" .. name]
	assert(#def.states == 7 and def.states[7].item_drop.amount == 1)
	assert(def.on_block_activated == nil)
end
assert(items["default:vegetable_stew"].container_item == "default:bowl")
print("farming growth, soil, harvest definitions, and fertilizer checks passed")
