local offsets = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}}
local function dry_out(pos, block, chunk, world)
	for _, offset in ipairs(offsets) do
		local state = world:get_block_state(pos.x + offset[1], pos.y + offset[2], pos.z + offset[3])
		if not state then return end
		local name = state:block():string_id()
		if name == "default:water" or name == "default:water_flowing" then return end
	end
	local name = block:string_id():gsub("default:", "default:dead_")
	world:set_block_from_str(pos.x, pos.y, pos.z, name)
end

for species, coral in ipairs({{"tube", "Tube"}, {"brain", "Brain"}, {"bubble", "Bubble"}, {"fire", "Fire"}, {"horn", "Horn"}}) do
	for _, dead in ipairs({false, true}) do
		local prefix = (dead and "dead_" or "") .. coral[1] .. "_coral"
		local label = (dead and "Dead " or "") .. coral[2] .. " Coral"
		mod:block {
			id = prefix .. "_block", name = label .. " Block", hardness = 1.5,
			tiles = "refi_" .. prefix .. "_block.png", harvest_requirements = Capability.Pickaxe,
			groups = {om_material_stone = 1, om_coral_species = species},
			item_drop = {id = "default:dead_" .. coral[1] .. "_coral_block", amount = 1},
			tick_randomly = not dead, tick_probability = 0.05, on_tick = not dead and dry_out or nil,
		}
		for _, fan in ipairs({false, true}) do
			local id = prefix .. (fan and "_fan" or "")
			mod:block {
				id = id, name = label .. (fan and " Fan" or ""), hardness = 0,
				tiles = "refi_" .. id .. ".png", inventory_image = "refi_" .. id .. ".png",
				draw_type = "xshape", is_opaque = false, is_collidable = false,
				bounding_box = {0, 0, 0, 1, 1, fan and 0.4 or 0.75},
				groups = {attached_node = 3, om_coral_plant = species, om_waterlogged = dead and 0 or 1, om_coral_fan = fan and 1 or 0},
				item_drop = {id = "default:dead_" .. coral[1] .. "_coral" .. (fan and "_fan" or ""), amount = 1},
				tick_randomly = not dead, tick_probability = 0.05, on_tick = not dead and dry_out or nil,
			}
		end
	end
end
