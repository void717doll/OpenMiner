#ifndef FARMINGTESTS_HPP_
#define FARMINGTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "server/lua/ScriptEngine.hpp"
#include "server/world/ServerBlock.hpp"

class FarmingTests : public CxxTest::TestSuite {
	public:
		void testCropLootReturnsProduceAndBoundedSeedsThroughLua() {
			Registry definitions;
			Registry::setInstance(definitions);
			definitions.registerItem<Item>({}, BLOCK_AIR, "air");
			definitions.registerItem<Item>({}, "test:seeds", "seeds");
			definitions.registerItem<Item>({}, "test:produce", "produce");
			auto &crop = definitions.registerBlock<ServerBlock>("test:crop");
			crop.addGroup("group:om_crop", 7);
			crop.getState(0).itemDrop("test:seeds");
			for (int age = 1; age <= 7; ++age) crop.addState();
			crop.getState(7).itemDrop("test:produce");
			unsigned matureSeeds = 0;
			for (int trial = 0; trial < 100; ++trial) {
				const auto ripe = crop.getState(7).getItemDrops();
				TS_ASSERT(!ripe.empty());
				if (ripe.empty()) continue;
				TS_ASSERT_EQUALS(ripe.front().stringID(), "test:produce");
				TS_ASSERT_EQUALS(ripe.front().amount(), 1);
				TS_ASSERT(ripe.size() <= 2);
				if (ripe.size() == 2) {
					TS_ASSERT_EQUALS(ripe[1].stringID(), "test:seeds");
					TS_ASSERT(ripe[1].amount() >= 1 && ripe[1].amount() <= 3);
					matureSeeds += ripe[1].amount();
				}
				for (const auto &drop : crop.getState(3).getItemDrops()) {
					TS_ASSERT_EQUALS(drop.stringID(), "test:seeds");
					TS_ASSERT(drop.amount() >= 1 && drop.amount() <= 3);
				}
			}
			TS_ASSERT(matureSeeds > 0);
			ScriptEngine scripts;
			scripts.init();
			scripts.lua()["crop"] = &crop.getState(7);
			auto result = scripts.lua().safe_script(
				"local drops = crop:get_item_drops(); assert(type(drops) == 'table'); "
				"assert(drops[1]:item():string_id() == 'test:produce'); "
				"for _, stack in ipairs(drops) do assert(stack:amount() > 0) end", sol::script_pass_on_error);
			TS_ASSERT(result.valid());
		}
};

#endif // FARMINGTESTS_HPP_
