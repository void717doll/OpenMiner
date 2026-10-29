local pieces = {"helmet", "chestplate", "leggings", "boots"}
local patterns = {{"###", "# #"}, {"# #", "###", "###"}, {"###", "# #", "# #"}, {"# #", "# #"}}
local points = {3, 8, 6, 3}
local durability = {11, 16, 15, 13}

for material, values in pairs({leather = {0, "leather"}, chain = {1}, iron = {2, "iron_ingot"}, diamond = {3, "diamond"}, gold = {1, "gold_ingot"}}) do
	for slot, piece in ipairs(pieces) do
		local id = piece .. "_" .. material
		local groups = {om_armor_slot = slot, om_armor_points = points[slot], om_max_damage = durability[slot] * 3 * 2 ^ values[1]}
		groups["om_armor_" .. piece] = 1
		mod:item {
			id = id,
			name = material .. " " .. piece,
			tiles = id .. ".png",
			max_stack_size = 1,
			groups = groups,
		}
		if values[2] then
			mod:crafting_recipe {
				result = {id = "default:" .. id, amount = 1},
				pattern = patterns[slot],
				keys = {["#"] = "default:" .. values[2]},
			}
		end
	end
end
