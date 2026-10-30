#ifndef BETAMININGTESTS_HPP_
#define BETAMININGTESTS_HPP_

#include <cxxtest/TestSuite.h>

#include "common/world/BetaMining.hpp"

class BetaMiningTests : public CxxTest::TestSuite {
	public:
		void testPickaxeHarvestTiers() {
			const char *materials[] = {"wooden", "stone", "iron", "golden", "diamond"};
			const u16 levels[] = {0, 1, 2, 0, 3};
			const char *blocks[] = {"stone", "iron_ore", "lapis_ore", "gold_ore", "diamond_ore", "redstone_ore", "obsidian"};
			const u16 requirements[] = {0, 2, 1, 2, 2, 2, 3};
			for (unsigned material = 0; material < 5; ++material) {
				Item pickaxe{1, {}, "default:" + std::string(materials[material]) + "_pickaxe", "pickaxe"};
				pickaxe.setHarvestCapability(1);
				pickaxe.addGroup("group:om_harvest_level", levels[material]);
				for (unsigned index = 0; index < 7; ++index) {
					Block block{1, "default:" + std::string(blocks[index])};
					block.getState(0).harvestRequirements(1);
					TS_ASSERT_EQUALS(retro::canHarvest(block.getState(0), pickaxe), levels[material] >= requirements[index]);
				}
			}
		}

		void testBareHandsAndWrongTools() {
			Item hands;
			Block stone{1, "default:stone"};
			stone.getState(0).hardness(1.5f);
			stone.getState(0).harvestRequirements(1);
			TS_ASSERT(!retro::canHarvest(stone.getState(0), hands));
			TS_ASSERT_DELTA(retro::breakTime(stone.getState(0), hands), 7.5f, 0.0001f);

			Item axe{1, {}, "default:diamond_axe", "axe"};
			axe.setHarvestCapability(4);
			axe.setMiningSpeed(8);
			axe.addGroup("group:om_harvest_level", 3);
			TS_ASSERT(!retro::canHarvest(stone.getState(0), axe));
			TS_ASSERT_DELTA(retro::breakTime(stone.getState(0), axe), 7.5f, 0.0001f);

			Block workbench{2, "default:workbench"};
			workbench.getState(0).harvestRequirements(4);
			TS_ASSERT(retro::canHarvest(workbench.getState(0), hands));
			Block dirt{3, "default:dirt"};
			TS_ASSERT(retro::canHarvest(dirt.getState(0), hands));
			TS_ASSERT_DELTA(retro::breakTime(dirt.getState(0), axe), 1.5f, 0.0001f);
		}

		void testSourceBreakTimesAndEffectiveGroups() {
			Item pickaxe{1, {}, "default:diamond_pickaxe", "pickaxe"};
			pickaxe.setHarvestCapability(1);
			pickaxe.setMiningSpeed(8);
			pickaxe.addGroup("group:om_harvest_level", 3);
			Block stone{1, "default:stone"};
			stone.getState(0).hardness(1.5f);
			stone.getState(0).harvestRequirements(1);
			TS_ASSERT_DELTA(retro::breakTime(stone.getState(0), pickaxe), 0.28125f, 0.0001f);

			Block obsidian{2, "default:obsidian"};
			obsidian.getState(0).hardness(10);
			obsidian.getState(0).harvestRequirements(1);
			TS_ASSERT_DELTA(retro::breakTime(obsidian.getState(0), pickaxe), 15.f, 0.0001f);

			Item shovel{2, {}, "default:stone_shovel", "shovel"};
			shovel.setMiningSpeed(4);
			shovel.addEffectiveBlock("group:om_material_dirt");
			Block dirt{3, "default:dirt"};
			dirt.addGroup("group:om_material_dirt", 1);
			dirt.getState(0).hardness(0.5f);
			TS_ASSERT_DELTA(retro::breakTime(dirt.getState(0), shovel), 0.1875f, 0.0001f);
		}

		void testUnbreakableAirLiquidsAndInstantBlocks() {
			Item hands;
			Block bedrock{1, "default:bedrock"};
			bedrock.getState(0).hardness(std::numeric_limits<float>::infinity());
			TS_ASSERT(!retro::canHarvest(bedrock.getState(0), hands));
			TS_ASSERT(std::isinf(retro::breakTime(bedrock.getState(0), hands)));
			bedrock.getState(0).hardness(-1);
			TS_ASSERT(std::isinf(retro::breakTime(bedrock.getState(0), hands)));
			Block water{2, "default:water"};
			water.getState(0).drawType(BlockDrawType::Liquid);
			TS_ASSERT(!retro::canHarvest(water.getState(0), hands));
			TS_ASSERT(std::isinf(retro::breakTime(water.getState(0), hands)));
			Block air{0, ""};
			TS_ASSERT(!retro::canHarvest(air.getState(0), hands));
			Block grass{3, "default:tallgrass"};
			grass.getState(0).hardness(0);
			TS_ASSERT_EQUALS(retro::breakTime(grass.getState(0), hands), 0.f);
		}

		void testAttackDamageUsesExistingItemGroups() {
			Item hands;
			TS_ASSERT_EQUALS(retro::attackDamage(hands), 1);
			const u16 damage[] = {4, 6, 8, 4, 10};
			for (u16 value : damage) {
				Item sword{1, {}, "default:sword", "sword"};
				sword.addGroup("group:om_attack_damage", value);
				TS_ASSERT_EQUALS(retro::attackDamage(sword), value);
			}
		}
};

#endif // BETAMININGTESTS_HPP_
