mod:block {
	id = "bed", name = "Bed", max_stack_size = 1,
	tiles = {
		"refi_bed_top_foot.png", "refi_bed_bottom_foot.png",
		"refi_bed_end_foot.png", "refi_bed_end_foot.png",
		"refi_bed_side_foot.png", "refi_bed_side_foot_r.png",
	},
	inventory_image = "refi_bed.png",
	draw_type = "boundingbox", bounding_box = {0, 0, 0, 1, 1, 9 / 16},
	is_opaque = false, is_rotatable = true, hardness = 0.2,
	groups = {om_material_wood = 1, attached_node = 3},
	item_drop = {id = "default:bed", amount = 1},
	states = {{tiles = {
		"refi_bed_top_head.png", "refi_bed_bottom_head.png",
		"refi_bed_end_head.png", "refi_bed_end_head.png",
		"refi_bed_side_head.png", "refi_bed_side_head_r.png",
	}}},
	on_block_activated = function(pos, block, player, world, client, server)
		server:use_bed(client.id, pos.x, pos.y, pos.z)
	end,
	on_block_destroyed = function(pos, world)
		world:remove_bed_partner(pos.x, pos.y, pos.z)
	end,
	tick_randomly = true, tick_probability = 0.05,
	on_tick = function(pos, block, chunk, world)
		world:validate_bed(pos.x, pos.y, pos.z)
	end,
}

mod:crafting_recipe {
	result = {id = "default:bed", amount = 1}, pattern = {"WWW", "PPP"},
	keys = {W = "default:wool", P = "default:oak_planks"},
}
