lush_caves = {}

local function loaded_state(world, x, y, z)
	if world:get_light(x, y, z) < 0 then return nil end
	return world:get_block_state(x, y, z)
end

local function vine_tick(pos, block, chunk, world)
	local state = loaded_state(world, pos.x, pos.y, pos.z)
	local below = loaded_state(world, pos.x, pos.y, pos.z - 1)
	if not state or not below or below:block():id() ~= 0 then return end
	local length = 1
	local anchor = loaded_state(world, pos.x, pos.y, pos.z + length)
	while anchor and anchor:block():string_id() == "default:cave_vines" and length <= 3 do
		length = length + 1
		anchor = loaded_state(world, pos.x, pos.y, pos.z + length)
	end
	if not anchor or not anchor:is_collidable() or length > 3 then return end
	if state:id() == 0 and math.random(8) == 1 then
		world:set_block_state(pos.x, pos.y, pos.z, 1)
	elseif length < 3 and math.random(4) == 1 then
		world:set_block_from_str(pos.x, pos.y, pos.z - 1, "default:cave_vines")
	end
end

local function harvest_berries(pos, block, player, world)
	local state = loaded_state(world, pos.x, pos.y, pos.z)
	if not state or state:id() ~= 1 then return end
	local remainder = mod:give_item_stack(player, ItemStack.new("default:glow_berries", 1))
	if remainder:amount() ~= 0 then return end
	world:set_block_state(pos.x, pos.y, pos.z, 0)
end

function lush_caves.fertilize(pos, world)
	local state = loaded_state(world, pos.x, pos.y, pos.z)
	if not state then return false end
	local id = state:block():string_id()
	if id == "default:cave_vines" then
		if state:id() ~= 0 then return false end
		world:set_block_state(pos.x, pos.y, pos.z, 1)
		return true
	end
	local offset, result
	if id == "default:rooted_dirt" then
		offset, result = -1, "default:hanging_roots"
	elseif id == "default:moss_block" then
		offset, result = 1, "default:moss_carpet"
	else
		return false
	end
	local target = loaded_state(world, pos.x, pos.y, pos.z + offset)
	if not target or target:block():id() ~= 0 then return false end
	world:set_block_from_str(pos.x, pos.y, pos.z + offset, result)
	return true
end

mod:block {
	id = "moss_block", name = "Moss", tiles = "refi_lush_moss_block.png",
	hardness = 0.1, groups = {om_material_dirt = 1},
}

mod:block {
	id = "moss_carpet", name = "Moss Carpet", tiles = "refi_lush_moss_carpet.png",
	hardness = 0.1, draw_type = "boundingbox", is_opaque = false, is_collidable = false,
	bounding_box = {0, 0, 0, 1, 1, 1 / 16},
	groups = {attached_node = 3, om_material_dirt = 1},
}

mod:block {
	id = "rooted_dirt", name = "Rooted Dirt", tiles = "refi_lush_rooted_dirt.png",
	hardness = 0.5, groups = {om_material_dirt = 1},
}

mod:block {
	id = "hanging_roots", name = "Hanging Roots", tiles = "refi_lush_hanging_roots.png",
	hardness = 0, draw_type = "xshape", is_opaque = false, is_collidable = false,
	groups = {attached_node = 4},
}

mod:item {
	id = "glow_berries", name = "Glow Berries", tiles = "refi_lush_glow_berries.png",
	heal_amount = 2, max_stack_size = 1,
}

mod:block {
	id = "cave_vines", name = "Cave Vines", tiles = "refi_lush_cave_vines.png",
	hardness = 0, draw_type = "xshape", is_opaque = false, is_collidable = false,
	groups = {attached_node = 4, om_hanging_chain = 1, om_light_level = 5},
	states = {{
		tiles = "refi_lush_cave_vines_lit.png", is_light_source = true,
		item_drop = {id = "default:glow_berries", amount = 1},
	}},
	tick_randomly = true, tick_probability = 0.001,
	on_tick = vine_tick, on_block_activated = harvest_berries,
}

for _, shrub in ipairs({
	{"azalea", "Azalea", "refi_lush_azalea_side.png"},
	{"flowering_azalea", "Flowering Azalea", "refi_lush_azalea_flowering_side.png"},
}) do
	mod:block {
		id = shrub[1], name = shrub[2], tiles = shrub[3],
		hardness = 0, draw_type = "xshape", is_opaque = false, is_collidable = false,
		groups = {attached_node = 3},
		placement_constraints = {
			[{0, 0, -1}] = {block = "group:om_material_dirt", is_whitelist = true},
		},
	}
end

mod:block {
	id = "spore_blossom", name = "Spore Blossom",
	tiles = {"refi_lush_spore_blossom_base.png", "refi_lush_spore_blossom.png", "refi_lush_spore_blossom_base.png"},
	hardness = 0, draw_type = "boundingbox", is_opaque = false, is_collidable = false,
	bounding_box = {1 / 8, 1 / 8, 13 / 16, 3 / 4, 3 / 4, 3 / 16},
	groups = {attached_node = 4, om_spore_blossom = 1},
}

mod:crafting_recipe {
	result = {id = "default:moss_carpet", amount = 3}, pattern = {"MM"},
	keys = {M = "default:moss_block"},
}

mod:crafting_recipe {
	result = {id = "default:cave_vines", amount = 1}, pattern = {"B"},
	keys = {B = "default:glow_berries"},
}
