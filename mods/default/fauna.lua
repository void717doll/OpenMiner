mod:block {
	id = "pumpkin", name = "Pumpkin",
	tiles = {"refi_pumpkin_top.png", "refi_pumpkin_top.png", "refi_pumpkin_face.png", "refi_pumpkin_side.png"},
	is_rotatable = true, hardness = 1,
	groups = {om_material_wood = 1},
}

mod:item {id = "rabbit_raw", name = "Raw Rabbit", tiles = "refi_rabbit_raw.png", heal_amount = 3, max_stack_size = 1}
mod:item {id = "rabbit_cooked", name = "Cooked Rabbit", tiles = "refi_rabbit_cooked.png", heal_amount = 5, max_stack_size = 1}
mod:item {id = "rabbit_hide", name = "Rabbit Hide", tiles = "refi_rabbit_hide.png"}
mod:item {id = "rabbit_foot", name = "Rabbit's Foot", tiles = "refi_rabbit_foot.png"}
mod:smelting_recipe {
	input = {id = "default:rabbit_raw", amount = 1}, output = {id = "default:rabbit_cooked", amount = 1},
}
mod:crafting_recipe {
	result = {id = "default:leather", amount = 1}, pattern = {"HH", "HH"}, keys = {H = "default:rabbit_hide"},
}
