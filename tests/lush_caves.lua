local blocks, items = {}, {}
local received, full = 0, false
mod = {
	block = function(_, def) blocks["default:" .. def.id] = def end,
	item = function(_, def) items["default:" .. def.id] = def end,
	crafting_recipe = function() end,
	give_item_stack = function(_, player, stack)
		if not full then received = received + stack:amount() end
		return {amount = function() return full and stack:amount() or 0 end}
	end,
}
ItemStack = {new = function(id, amount)
	assert(id == "default:glow_berries" and amount == 1)
	return {amount = function() return amount end}
end}
dofile("mods/default/lush_caves.lua")

local cells, unloaded = {}, false
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
	get_light = function(_, x, y, z) return unloaded and z < 8 and -1 or 0 end,
	get_block_state = function(_, x, y, z) return cells[key(x, y, z)] or air end,
	set_block_state = function(_, x, y, z, age) cells[key(x, y, z)].age = age end,
	set_block_from_str = function(_, x, y, z, id) place(x, y, z, id) end,
}
local vine = blocks["default:cave_vines"]
local pos = {x = 0, y = 0, z = 9}
place(0, 0, 10, "default:rooted_dirt", 0, true)
local state = place(0, 0, 9, "default:cave_vines", 1)
full = true
vine.on_block_activated(pos, nil, {}, world)
assert(state.age == 1 and received == 0, "full inventory must preserve berries")
full = false
vine.on_block_activated(pos, nil, {}, world)
vine.on_block_activated(pos, nil, {}, world)
assert(state.age == 0 and received == 1, "one harvest must give exactly one berry")
assert(lush_caves.fertilize(pos, world) and state.age == 1)
assert(not lush_caves.fertilize(pos, world))

local random = math.random
math.random = function(maximum) return maximum == 4 and 1 or 2 end
vine.on_tick(pos, nil, nil, world)
assert(world:get_block_state(0, 0, 8):block():string_id() == "default:cave_vines")
pos.z = 8
unloaded = true
vine.on_tick(pos, nil, nil, world)
assert(world:get_block_state(0, 0, 7):block():id() == 0)
unloaded = false
vine.on_tick(pos, nil, nil, world)
assert(world:get_block_state(0, 0, 7):block():string_id() == "default:cave_vines")
pos.z = 7
vine.on_tick(pos, nil, nil, world)
assert(world:get_block_state(0, 0, 6):block():id() == 0, "vines must stop at three blocks")
math.random = function() return 1 end
vine.on_tick(pos, nil, nil, world)
assert(world:get_block_state(0, 0, 7):id() == 1, "a mature short vine can regrow berries")
math.random = random

pos = {x = 2, y = 0, z = 9}
place(2, 0, 9, "default:rooted_dirt", 0, true)
place(2, 0, 8, "default:copper_ore", 0, true)
assert(not lush_caves.fertilize(pos, world))
assert(world:get_block_state(2, 0, 8):block():string_id() == "default:copper_ore")
cells[key(2, 0, 8)] = nil
assert(lush_caves.fertilize(pos, world))
assert(world:get_block_state(2, 0, 8):block():string_id() == "default:hanging_roots")
place(2, 0, 9, "default:moss_block", 0, true)
assert(lush_caves.fertilize(pos, world))
assert(world:get_block_state(2, 0, 10):block():string_id() == "default:moss_carpet")
assert(not lush_caves.fertilize(pos, world))
assert(items["default:glow_berries"].heal_amount == 2 and items["default:glow_berries"].max_stack_size == 1)
assert(vine.groups.om_light_level == 5 and vine.states[1].item_drop.id == "default:glow_berries")
print("lush vine growth, loaded boundaries, harvest, food and fertilizer checks passed")
