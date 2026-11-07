mod:block {
	id = "oak_fence", name = "Oak Fence",
	tiles = "oak_planks.png", hardness = 2,
	draw_type = "boundingbox", is_opaque = false,
	bounding_box = {0, 0, 0, 1, 1, 1},
	groups = {om_material_wood = 1, fence = 1, om_fuel = 300},
}

mod:block {
	id = "cobweb", name = "Cobweb",
	tiles = "refi_cobweb.png", inventory_image = "refi_cobweb.png",
	draw_type = "xshape", hardness = 4,
	is_opaque = false, is_collidable = false,
	groups = {om_web = 1},
	item_drop = {id = "default:string", amount = 1},
}

mod:crafting_recipe {
	result = {id = "default:oak_fence", amount = 2},
	pattern = {"SSS", "SSS"}, keys = {S = "default:stick"},
}
