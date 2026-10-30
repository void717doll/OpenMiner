local frames = {}
for frame = 0, 63 do frames[#frames + 1] = string.format("refi_clock_%02d.png", frame) end

mod:item {
	id = "clock", name = "Clock", tiles = frames,
	groups = {om_clock = 1},
}

mod:crafting_recipe {
	result = {id = "default:clock", amount = 1},
	pattern = {" G ", "GRG", " G "},
	keys = {G = "default:gold_ingot", R = "default:redstone_dust"},
}
