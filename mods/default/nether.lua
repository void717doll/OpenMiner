mod:block {
	id = "basalt",
	name = "Basalt",
	tiles = {"expansion_nether_basalt.png", "expansion_nether_basalt_side.png"},
	hardness = 1.5,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1, om_min_harvest_level = 1},
}

mod:block {
	id = "chiseled_basalt",
	name = "Chiseled Basalt",
	tiles = {"expansion_nether_basalt_chiselled_top.png", "expansion_nether_basalt_chiselled_side.png"},
	hardness = 1.5,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1, om_min_harvest_level = 1},
}

for _, brick in ipairs({{"nether_bricks", "Nether Bricks", "brick"}, {"cracked_nether_bricks", "Cracked Nether Bricks", "brick_cracked"}}) do
	mod:block {
		id = brick[1],
		name = brick[2],
		tiles = "expansion_nether_" .. brick[3] .. ".png",
		hardness = 2,
		harvest_requirements = Capability.Pickaxe,
		groups = {om_material_stone = 1},
	}
end

mod:item {
	id = "quartz",
	name = "Nether Quartz",
	tiles = "expansion_quartz.png",
}

mod:block {
	id = "quartz_ore",
	name = "Nether Quartz Ore",
	tiles = "expansion_quartz_ore.png",
	hardness = 3,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1, om_min_harvest_level = 1},
	item_drop = {id = "default:quartz", amount = 1},
}

mod:block {
	id = "quartz_block",
	name = "Quartz Block",
	tiles = "expansion_quartz_block.png",
	hardness = 0.8,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1},
}

mod:block {
	id = "quartz_pillar",
	name = "Quartz Pillar",
	tiles = {"expansion_quartz_pillar_top.png", "expansion_quartz_pillar_side.png"},
	is_rotatable = true,
	hardness = 0.8,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1},
}

mod:block {
	id = "chiseled_quartz",
	name = "Chiseled Quartz",
	tiles = "expansion_quartz_chiseled.png",
	hardness = 0.8,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1},
}

mod:smelting_recipe {
	input = {id = "default:netherrack", amount = 1},
	output = {id = "default:nether_bricks", amount = 1},
}

mod:smelting_recipe {
	input = {id = "default:nether_bricks", amount = 1},
	output = {id = "default:cracked_nether_bricks", amount = 1},
}

mod:crafting_recipe {
	result = {id = "default:chiseled_basalt", amount = 4},
	pattern = {"BB", "BB"},
	keys = {B = "default:basalt"},
}

mod:crafting_recipe {
	result = {id = "default:quartz_block", amount = 1},
	pattern = {"QQ", "QQ"},
	keys = {Q = "default:quartz"},
}

mod:crafting_recipe {
	result = {id = "default:quartz_pillar", amount = 2},
	pattern = {"Q", "Q"},
	keys = {Q = "default:quartz_block"},
}

mod:crafting_recipe {
	result = {id = "default:chiseled_quartz", amount = 4},
	pattern = {"QQ", "QQ"},
	keys = {Q = "default:quartz_block"},
}
