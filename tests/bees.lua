local blocks, items, recipes = {}, {}, {}
local received, full = 0, false
mod = {
	block = function(_, def) blocks["default:" .. def.id] = def end,
	item = function(_, def) items["default:" .. def.id] = def end,
	crafting_recipe = function(_, def) recipes[#recipes + 1] = def end,
	smelting_recipe = function() end,
	give_item_stack = function(_, player, stack)
		if not full then received = received + stack:amount() end
		return {amount = function() return full and stack:amount() or 0 end}
	end,
}
ItemStack = {new = function(id, amount)
	return {amount = function() return amount end, item = function() return {string_id = function() return id end} end}
end}
dofile("mods/default/farming.lua")
dofile("mods/default/bees.lua")

local cells, data, changed = {}, {}, {}
local now, day, light, dimension = 0, true, 15, 0
local function key(x, y, z) return x .. "," .. y .. "," .. z end
local function place(x, y, z, id, age)
	local block = {string_id = function() return id end, id = function() return id == "air" and 0 or 1 end}
	local state = {age = age or 0, block = function() return block end}
	state.id = function(self) return self.age end
	state.is_collidable = function() return id == "default:stone" or id == "default:campfire" end
	cells[key(x, y, z)] = state
	return state
end
local air = place(-99, 0, 0, "air")
local world = {
	emit_particles = function() end,
	get_light = function() return light end,
	get_ticks = function() return now end,
	is_daytime = function() return day end,
	dimension = function() return {id = function() return dimension end} end,
	get_block_state = function(_, x, y, z) return cells[key(x, y, z)] or air end,
	set_block_state = function(_, x, y, z, age) cells[key(x, y, z)].age = age end,
	get_block_data = function(_, x, y, z) return data[key(x, y, z)] end,
	add_block_data = function(_, x, y, z)
		local values = {}
		local meta = {
			get_int = function(_, name) return values[name] end,
			set_int = function(_, name, value) values[name] = value end,
			get_string = function(_, name) return values[name] end,
			set_string = function(_, name, value) values[name] = value end,
		}
		data[key(x, y, z)] = {meta = meta}
		return data[key(x, y, z)]
	end,
	mark_block_modified = function(_, x, y, z) changed[key(x, y, z)] = true end,
}
local pos = {x = 0, y = 0, z = 1}
local other = {x = 1, y = 0, z = 1}
local wild = blocks["default:wild_hive"]
local block = {string_id = function() return "default:wild_hive" end}
local function tick(position)
	wild.on_tick(position or pos, block, nil, world)
end
local function meta(position)
	return world:get_block_data(position.x, position.y, position.z).meta
end
place(2, 0, 1, "default:rose")
place(3, 0, 1, "default:dandelion")
place(0, 1, 0, "default:farmland", 7)
local crop = place(0, 1, 1, "default:tomato_seeds", 0)
for _ = 1, 24 do now = now + 200 tick() end
assert(meta(pos):get_int("bee_honey") == 1)
assert(crop.age == 2)
assert(changed[key(0, 0, 1)] and changed[key(0, 1, 1)])
tick(other)
assert(crop.age == 2, "overlapping hives must share crop cooldown")
for _ = 1, 72 do now = now + 200 tick() end
assert(meta(pos):get_int("bee_honey") == 3)
assert(crop.age == 7)

local progress = meta(other):get_int("bee_progress")
now = now + 200 day = false tick(other)
assert(meta(other):get_int("bee_progress") == progress and meta(other):get_int("bee_active") == 0)
day = true dimension = 1 now = now + 200 tick(other)
assert(meta(other):get_int("bee_progress") == progress)
dimension = 0 light = 11 now = now + 200 tick(other)
assert(meta(other):get_int("bee_progress") == progress)
light = 15 cells[key(3, 0, 1)] = nil now = now + 200 tick(other)
assert(meta(other):get_int("bee_progress") == progress)
place(3, 0, 1, "default:dandelion")
now = now + 24000 tick(other)
assert(meta(other):get_int("bee_progress") == progress + 200, "unloaded time must not generate honey")

local held = ItemStack.new("default:stick", 1)
local player = {held_item_stack = function() return held end}
local client = {id = 7, creative = false}
local stings, wear = 0, 0
local server = {
	damage_player = function(_, id, damage, cause)
		assert(id == client.id and damage == 2 and cause == "bees")
		stings = stings + 1
	end,
	wear_held_item = function(_, target, amount) assert(target == player) wear = wear + amount end,
}
local function harvest() wild.on_block_activated(pos, block, player, world, client, server) end
full = true harvest()
assert(received == 0 and stings == 0 and meta(pos):get_int("bee_honey") == 3)
full = false harvest()
assert(received == 1 and stings == 1 and meta(pos):get_int("bee_honey") == 2)
held = ItemStack.new("default:bee_smoker", 1)
harvest() harvest()
assert(wear == 1 and meta(pos):get_int("bee_honey") == 2)
held = ItemStack.new("default:stick", 1)
now = now + 100 harvest()
assert(received == 2 and stings == 1)
now = now + 1000 harvest()
assert(received == 3 and stings == 2)
harvest()
assert(received == 3 and stings == 2)
meta(pos):set_int("bee_honey", 1)
client.creative = true harvest()
assert(received == 4 and stings == 2)
items["default:bee_smoker"].on_item_activated(pos, air:block(), player, world, client, server)
assert(wear == 1)
client.creative = false
now = now + 1000
place(0, 0, -2, "default:campfire", 0)
meta(pos):set_int("bee_honey", 1)
harvest()
assert(received == 5 and stings == 2, "a lit campfire beneath the hive must prevent stings")
place(0, 0, -2, "default:campfire", 1)
meta(pos):set_int("bee_honey", 1)
harvest()
assert(received == 6 and stings == 3, "an extinguished campfire must not calm bees")
now = now + 1000
place(0, 0, -2, "default:campfire", 0)
place(0, 0, -1, "default:stone")
meta(pos):set_int("bee_honey", 1)
harvest()
assert(received == 7 and stings == 4, "a solid ceiling must block campfire smoke")
now = now + 1000
cells[key(0, 0, -1)] = nil
cells[key(0, 0, -2)] = nil
place(0, 0, -5, "default:campfire", 0)
meta(pos):set_int("bee_honey", 1)
harvest()
assert(received == 8 and stings == 5, "campfire smoke must stay within five blocks")
assert(wild.groups.attached_node == 3 and wild.item_drop.id == "default:honeycomb")
assert(blocks["default:beehive"].item_drop.id == "default:beehive")
assert(items["default:honey"].heal_amount == 4 and items["default:honey"].max_stack_size == 1)
print("bees production, capped pollination, daylight, persistence marking, harvest, smoke, and sting checks passed")
