/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */
#ifndef BLOCKPARAMTESTS_HPP_
#define BLOCKPARAMTESTS_HPP_

#include <cxxtest/TestSuite.h>

#include "Block.hpp"
#include "BlockParam.hpp"

class BlockParamTests : public CxxTest::TestSuite  {
	public:
		void testFullCustomData() {
			Block block{1, "default:custom"};
			block.param().allocateBits(BlockParam::Custom, 16);
			TS_ASSERT_EQUALS(block.param().getParam(BlockParam::Custom,
				block.param().setParam(BlockParam::Custom, 0, 65535)), 65535);
			TS_ASSERT_EQUALS(BlockParam::getTypeName(BlockParam::Custom), "Custom");
			TS_ASSERT_THROWS_ANYTHING(block.param().allocateBits(BlockParam::Rotation, 1));
		}

		void testStateInheritance() {
			Block block{1, "default:stateful"};
			block.getState(0).hardness(4.f);
			block.getState(0).harvestRequirements(2);
			block.addState();
			TS_ASSERT_EQUALS(block.getState(1).hardness(), 4.f);
			TS_ASSERT_EQUALS(block.getState(1).timeToBreak(0, 1.f, false), 20.f);
			block.getState(1).hardness(2.f);
			TS_ASSERT_EQUALS(block.getState(1).timeToBreak(0, 1.f, false), 10.f);
		}

		void testAllocation() {
			Block m_block{1, "default:myblock"};

			TS_ASSERT_EQUALS(m_block.param().getAllocatedSize(), 0);

			m_block.param().allocateBits(BlockParam::Rotation, 5);
			m_block.param().allocateBits(BlockParam::State, 2);

			u16 blockParam = 0;

			blockParam = m_block.param().setParam(BlockParam::Rotation, blockParam, 16);
			TS_ASSERT_EQUALS(blockParam, 16);

			blockParam = m_block.param().setParam(BlockParam::State, blockParam, 3);
			TS_ASSERT_EQUALS(blockParam, 16 | (3 << 5));

			TS_ASSERT_EQUALS(m_block.param().getParam(BlockParam::Rotation, blockParam), 16);
			TS_ASSERT_EQUALS(m_block.param().getParam(BlockParam::State, blockParam), 3);
		}
};

#endif // BLOCKPARAMTESTS_HPP_
