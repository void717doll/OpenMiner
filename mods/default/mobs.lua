-- health, dimensions, movement and drops follow the supplied beta entity classes.
local definitions = {
	{"pig", 10, 0.7, 0, "porkchop_raw", 0.9, 0.9, false},
	{"zombie", 20, 0.5, 5, "feather", 0.6, 1.8, true},
	{"cow", 10, 0.7, 0, "leather", 0.9, 1.3, false},
	{"sheep", 10, 0.7, 0, "wool", 0.9, 1.3, false},
	{"chicken", 4, 0.7, 0, "feather", 0.3, 0.4, false},
	{"skeleton", 20, 0.7, 2, "arrow", 0.6, 1.8, true},
	{"creeper", 20, 0.7, 0, "gunpowder", 0.6, 1.8, true},
	{"spider", 20, 0.8, 2, "string", 1.4, 0.9, true},
	{"slime", 4, 0.6, 2, "slimeball", 1.2, 1.2, true},
	{"ghast", 10, 0.7, 0, "gunpowder", 4, 4, true},
	{"pig_zombie", 20, 0.5, 5, "porkchop_cooked", 0.6, 1.8, false},
	{"wolf", 8, 1.1, 2, "", 0.8, 0.8, false},
	{"squid", 10, 0.2, 0, "ink_sac", 0.95, 0.95, false},
	{"giant", 200, 0.5, 50, "", 3.6, 10.8, true},
}

for _, definition in ipairs(definitions) do
	local width, height = definition[6], definition[7]
	mod:entity {
		id = definition[1],
		properties = {
			visual = {type = "retro_mob", model = definition[1]},
			is_rotatable = true,
			hitbox = {-width / 2, -width / 2, 0, width, width, height},
			mob = {
				kind = definition[1], health = definition[2], move_speed = definition[3],
				attack_strength = definition[4], hostile = definition[8],
				drop_item = definition[5] ~= "" and "default:" .. definition[5] or "",
			},
		},
	}
end
