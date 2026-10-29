-- values and models adapted from retromcp beta 1.7.3.
mod:entity {
	id = "pig",
	properties = {
		visual = {type = "retro_mob", model = "pig"},
		is_rotatable = true,
		hitbox = {-0.45, -0.45, 0, 0.9, 0.9, 0.9},
		mob = {health = 10, move_speed = 0.7, drop_item = "default:porkchop_raw"},
	},
}

mod:entity {
	id = "zombie",
	properties = {
		visual = {type = "retro_mob", model = "zombie"},
		is_rotatable = true,
		hitbox = {-0.3, -0.3, 0, 0.6, 0.6, 1.8},
		mob = {
			hostile = true,
			health = 20,
			move_speed = 0.5,
			attack_strength = 5,
			drop_item = "default:feather",
		},
	},
}
