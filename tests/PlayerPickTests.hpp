#ifndef PLAYERPICKTESTS_HPP_
#define PLAYERPICKTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/Registry.hpp"
#include "common/world/Player.hpp"

class PlayerPickTests : public CxxTest::TestSuite {
	Registry m_registry;
public:
	void setUp() override {
		Registry::setInstance(m_registry);
		m_registry.registerItem<Item>({}, BLOCK_AIR, "air");
		m_registry.registerItem<Item>({}, "test:stone", "stone");
		m_registry.registerItem<Item>({}, "test:dirt", "dirt");
	}
	void tearDown() override { m_registry.clear(); }
	void testHotbarSelectionPreservesOwnedStack() {
		Player player; player.setHeldItemSlot(0);
		player.inventory().setStack(5, 0, "test:stone", 23);
		TS_ASSERT(player.pickItem("test:stone"));
		TS_ASSERT_EQUALS(player.heldItemSlot(), 5);
		TS_ASSERT_EQUALS(player.heldItemStack().amount(), 23);
		TS_ASSERT_EQUALS(player.inventory().getStack(0, 0).amount(), 0);
	}
	void testSurvivalMovesOwnedStackWithoutCreatingItems() {
		Player player; player.setHeldItemSlot(2);
		for (u16 slot=0;slot<9;++slot) player.inventory().setStack(slot,0,"test:dirt",10);
		player.inventory().setItemStack(4, 2, ItemStack{"test:stone", 7, 12});
		TS_ASSERT(player.pickItem("test:stone"));
		TS_ASSERT_EQUALS(player.heldItemSlot(), 2);
		TS_ASSERT_EQUALS(player.heldItemStack().amount(), 7);
		TS_ASSERT_EQUALS(player.heldItemStack().damage(), 12);
		TS_ASSERT_EQUALS(player.inventory().getStack(4, 2).stringID(), "test:dirt");
		TS_ASSERT_EQUALS(player.inventory().getStack(4, 2).amount(), 10);
	}
	void testSurvivalCannotCreateMissingBlock() {
		Player player; player.setHeldItemSlot(3);
		TS_ASSERT(!player.pickItem("test:stone"));
		TS_ASSERT_EQUALS(player.heldItemSlot(), 3);
		for (const auto &stack : player.inventory().items()) TS_ASSERT_EQUALS(stack.amount(), 0);
	}
	void testCreativePrefersEmptySlotAndRejectsInvalidItems() {
		Player player; player.setCreative(true); player.setHeldItemSlot(0);
		player.inventory().setStack(0, 0, "test:dirt", 2);
		TS_ASSERT(player.pickItem("test:stone"));
		TS_ASSERT_EQUALS(player.heldItemSlot(), 1);
		TS_ASSERT_EQUALS(player.heldItemStack().amount(), m_registry.getItemFromStringID("test:stone").maxStackSize());
		TS_ASSERT_EQUALS(player.inventory().getStack(0, 0).amount(), 2);
		TS_ASSERT(!player.pickItem("missing:item"));
		TS_ASSERT(!player.pickItem(BLOCK_AIR));
	}
};
#endif
