#ifndef HEARTBARTESTS_HPP_
#define HEARTBARTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/core/HeartBarState.hpp"

class HeartBarTests : public CxxTest::TestSuite {
	public:
		void testEveryHealthValue() {
			for (unsigned health = 0; health <= 20; ++health) {
				unsigned displayedHealth = 0;
				unsigned halfHearts = 0;
				for (unsigned slot = 0; slot < hearts::count; ++slot) {
					auto state = hearts::fill(health, slot);
					if (state == hearts::Fill::Full)
						displayedHealth += 2;
					else if (state == hearts::Fill::Half) {
						++displayedHealth;
						++halfHearts;
					}
					if (slot > 0 && state != hearts::Fill::Empty)
						TS_ASSERT(hearts::fill(health, slot - 1) == hearts::Fill::Full);
				}
				TS_ASSERT_EQUALS(displayedHealth, health);
				TS_ASSERT_EQUALS(halfHearts, health % 2);
			}
		}

		void testBoundarySlots() {
			TS_ASSERT(hearts::fill(0, 0) == hearts::Fill::Empty);
			TS_ASSERT(hearts::fill(1, 0) == hearts::Fill::Half);
			TS_ASSERT(hearts::fill(19, 9) == hearts::Fill::Half);
			TS_ASSERT(hearts::fill(20, 9) == hearts::Fill::Full);
			TS_ASSERT(hearts::fill(65535, 9) == hearts::Fill::Full);
			TS_ASSERT(hearts::fill(20, 10) == hearts::Fill::Empty);
		}
};

#endif // HEARTBARTESTS_HPP_
