--
-- =====================================================================================
--
--  OpenMiner
--
--  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
--  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
--
--  This file is part of OpenMiner.
--
--  OpenMiner is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public
--  License as published by the Free Software Foundation; either
--  version 2.1 of the License, or (at your option) any later version.
--
--  OpenMiner is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public License
--  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
--  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
--
-- =====================================================================================
--

mod:biome {
	id = "grassland",
	name = "Grassland",

	params = {
		temperature = 0.5,
		precipitation = 0.3
	},

	blocks = {
		top = "default:grass",
		ground = "default:dirt",
		deep = "default:stone",
		beach = "default:sand",
		liquid = "default:water",
		portal = "default:portal",
		portal_frame = "default:obsidian",
	},

	trees = {
		{
			type = "default:oak",
			probability = 0.01041666667
		}
	},

	-- Currently, all ores are defined per-biome. In a later update,
	-- we could see ores defined mostly by worldtype, and some extras
	-- added per biome. In addition, given the large world depth, we
	-- could decide whether ores at a point very far down should
	-- still depend on the surface biome, or only on the worldtype.
	ores = {
		{
			block = "default:coal_ore",
			probability = 0.001,
			size = 2,
			gen_type = "flood_fill",
		},
		{
			block = "default:iron_ore",
			probability = 0.003,
			size = 8,
			gen_type = "random_walk",
		}
	},

	flora = {
		{block = "default:fern", spawns_on = "default:grass", probability = 0.035},
		{block = "default:rose", spawns_on = "default:grass", probability = 0.015},
		{block = "default:mushroom_brown", spawns_on = "default:grass", probability = 0.006},
		{block = "default:mushroom_red", spawns_on = "default:grass", probability = 0.004},
		{
			block = "default:tallgrass",
			spawns_on = "default:grass",
			probability = 0.25
		},
		{
			block = "default:dandelion",
			spawns_on = "default:grass",
			probability = 0.025
		}
	}
}

mod:biome {
	id = "desert",
	name = "Desert",

	params = {
		temperature = -0.3,
		precipitation = -0.7
	},

	blocks = {
		top = "default:sand",
		ground = "default:sand",
		deep = "default:stone",
		beach = "default:sand",
		liquid = "default:water",
		portal = "default:portal",
		portal_frame = "default:obsidian",
	},

	trees = {
		{
			type = "default:tree_cactus",
			probability = 0.001,
		}
	},

	flora = {
		{
			block = "default:deadbush",
			spawns_on = "default:sand",
			probability = 0.005,
		}
	},
}

mod:biome {
	id = "netherland",
	name = "Netherland",

	params = {
		temperature = -0.3,
		precipitation = -1
	},

	blocks = {
		top = "default:netherrack",
		ground = "default:netherrack",
		deep = "default:netherrack",
		beach = "default:soul_sand",
		liquid = "default:lava",
		portal = "default:portal",
		portal_frame = "default:obsidian",
	},
}


-- keep earlier biome ids stable for saved worlds.
local beta_biomes = {
	{"rainforest", "Rainforest", 0.03, 0.3},
	{"swampland", "Swampland", 0.01, 0.15},
	{"seasonal_forest", "Seasonal Forest", 0.02, 0.2},
	{"forest", "Forest", 0.025, 0.2},
	{"savanna", "Savanna", 0.001, 0.15},
	{"shrubland", "Shrubland", 0.005, 0.2},
	{"taiga", "Taiga", 0.02, 0.1},
	{"plains", "Plains", 0.001, 0.25},
	{"tundra", "Tundra", 0.001, 0.05},
	{"ice_desert", "Ice Desert", 0, 0},
}
for _, biome in ipairs(beta_biomes) do
	local sand = biome[1] == "ice_desert"
	mod:biome {
		id = biome[1],
		name = biome[2],
		params = {temperature = 0.5, precipitation = 0.5},
		blocks = {
			top = sand and "default:sand" or "default:grass",
			ground = sand and "default:sand" or "default:dirt",
			deep = "default:stone", beach = "default:sand", liquid = "default:water",
			portal = "default:portal", portal_frame = "default:obsidian",
		},
		trees = biome[3] > 0 and {{type = "default:oak", probability = biome[3]}} or {},
		flora = biome[4] > 0 and {{block = "default:tallgrass", spawns_on = "default:grass", probability = biome[4]}} or {},
		ores = {
			{block = "default:coal_ore", probability = 0.001, size = 2, gen_type = "flood_fill"},
			{block = "default:iron_ore", probability = 0.003, size = 8, gen_type = "random_walk"},
		},
	}
end
