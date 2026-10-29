mod:block {
	id = "copper_ore",
	name = "Copper Ore",
	tiles = "copper_ore.png",
	hardness = 3,
	harvest_requirements = Capability.Pickaxe,
	groups = {om_material_stone = 1},
}

mod:item {
	id = "copper_ingot",
	name = "Copper Ingot",
	tiles = "copper_ingot.png",
}

mod:smelting_recipe {
	input = {id = "default:copper_ore", amount = 1},
	output = {id = "default:copper_ingot", amount = 1},
}

local tools = {
	{"pickaxe", Capability.Pickaxe, {"CCC", " S ", " S "}},
	{"axe", Capability.Axe, {"CC", "CS", " S"}},
	{"shovel", Capability.Shovel, {"C", "S", "S"}},
	{"hoe", 0, {"CC", " S", " S"}},
	{"sword", 0, {"C", "C", "S"}},
}
for _, tool in ipairs(tools) do
	register_tool(tool[1], "copper", 5, tool[2], 1, 190)
	mod:crafting_recipe {
		result = {id = "default:copper_" .. tool[1], amount = 1},
		pattern = tool[3],
		keys = {C = "default:copper_ingot", S = "default:stick"},
	}
end
