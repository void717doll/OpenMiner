mod:block {
	id = "tnt",
	name = "TNT",
	tiles = {"tnt_top.png", "tnt_bottom.png", "tnt_side.png"},
	hardness = 0,
}

mod:item {
	id = "flint_and_steel",
	name = "Flint and Steel",
	tiles = "flint_and_steel.png",
	max_stack_size = 1,
	groups = {om_igniter = 1, om_max_damage = 64},
}

mod:crafting_recipe {
	result = {id = "default:tnt", amount = 1},
	pattern = {"GSG", "SGS", "GSG"},
	keys = {G = "default:gunpowder", S = "default:sand"},
}

mod:crafting_recipe {
	result = {id = "default:flint_and_steel", amount = 1},
	pattern = {"I ", " F"},
	keys = {I = "default:iron_ingot", F = "default:flint"},
}
