local slabs = {
	{"stone_slab", "Stone Slab", "stone", {"refi_stone_slab_top.png", "refi_stone_slab_top.png", "refi_stone_slab_side.png"}},
	{"sandstone_slab", "Sandstone Slab", "sandstone", {"sandstone_top.png", "sandstone_bottom.png", "sandstone_normal.png"}},
	{"cobblestone_slab", "Cobblestone Slab", "cobblestone", "cobblestone.png"},
}

for _, slab in ipairs(slabs) do
	mod:block {
		id = slab[1], name = slab[2], tiles = slab[4],
		hardness = 2, harvest_requirements = Capability.Pickaxe,
		groups = {om_material_stone = 1, om_slab = 1},
		draw_type = "boundingbox", is_opaque = false,
		bounding_box = {0, 0, 0, 1, 1, 0.5},
		states = {{
			bounding_box = {0, 0, 0, 1, 1, 1}, is_opaque = true,
			item_drop = {id = "default:" .. slab[1], amount = 2},
		}},
	}
	mod:crafting_recipe {
		result = {id = "default:" .. slab[1], amount = 3},
		pattern = {"SSS"}, keys = {S = "default:" .. slab[3]},
	}
end
