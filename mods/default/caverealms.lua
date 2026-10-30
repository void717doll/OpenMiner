for _, covering in ipairs({{"moss", "Mossy Cave Stone"}, {"lichen", "Lichen Cave Stone"}}) do
	mod:block {
		id = "cave_" .. covering[1],
		name = covering[2],
		tiles = {"expansion_cave_" .. covering[1] .. ".png", "stone.png", "stone.png"},
		hardness = 1.5,
		harvest_requirements = Capability.Pickaxe,
		groups = {om_material_stone = 1},
	}
end

mod:block {
	id = "cave_fungi",
	name = "Cave Fungi",
	tiles = "expansion_cave_fungi.png",
	hardness = 0,
	draw_type = "xshape",
	is_opaque = false,
	is_collidable = false,
	groups = {attached_node = 3, om_cave_decoration = 1},
}

mod:item {
	id = "mycena_powder",
	name = "Mycena Powder",
	tiles = "expansion_mycena_powder.png",
}

mod:block {
	id = "cave_mycena",
	name = "Mycena",
	tiles = "expansion_cave_mycena.png",
	hardness = 0,
	draw_type = "xshape",
	is_opaque = false,
	is_collidable = false,
	is_light_source = true,
	groups = {attached_node = 3, om_cave_decoration = 1, om_light_level = 3},
	item_drop = {id = "default:mycena_powder", amount = 1},
}

mod:block {
	id = "cave_glowworm",
	name = "Glowworms",
	tiles = "expansion_cave_glow_worm.png",
	hardness = 0,
	draw_type = "xshape",
	is_opaque = false,
	is_collidable = false,
	is_light_source = true,
	groups = {attached_node = 3, om_cave_decoration = 1, om_light_level = 4},
}

mod:block {
	id = "cave_crystal",
	name = "Cave Crystal",
	tiles = "expansion_cave_glow_gem.png",
	hardness = 1,
	draw_type = "xshape",
	is_opaque = false,
	is_collidable = false,
	is_light_source = true,
	harvest_requirements = Capability.Pickaxe,
	groups = {attached_node = 3, om_cave_decoration = 1, om_material_stone = 1, om_min_harvest_level = 1, om_light_level = 5},
}

mod:block {
	id = "crystal_lamp",
	name = "Crystal Lamp",
	tiles = "expansion_cave_glow_crystal.png",
	hardness = 0.5,
	is_light_source = true,
	groups = {om_light_level = 14},
}

mod:crafting_recipe {
	result = {id = "default:crystal_lamp", amount = 1},
	pattern = {" C ", "CGC", " C "},
	keys = {C = "default:cave_crystal", G = "default:glass"},
}
