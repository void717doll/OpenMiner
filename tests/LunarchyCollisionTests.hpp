#ifndef LUNARCHYCOLLISIONTESTS_HPP_
#define LUNARCHYCOLLISIONTESTS_HPP_

#include <cxxtest/TestSuite.h>
#include "common/world/LunarchyCollision.hpp"

class LunarchyCollisionTests : public CxxTest::TestSuite {
	public:
		void testHighSpeedCannotTunnelThroughThinWall() {
			std::vector<collision::Obstacle> walls{{DoubleBox{5, -10, -10, 0.125, 20, 20}}};
			auto result = collision::sweep(walls, {0, 0, 0, 0.6, 0.6, 1.8}, {100, 0, 0}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(result.movement.x, 4.4, 1e-9);
			TS_ASSERT_DELTA(result.velocity.x, 0, 1e-9);
			TS_ASSERT(result.horizontalCollision);
		}

		void testEarliestContactPreservesWallSlide() {
			std::vector<collision::Obstacle> walls{{DoubleBox{2, -10, -10, 1, 20, 20}}};
			auto result = collision::sweep(walls, {0, 0, 0, 1, 1, 1}, {4, 3, 0}, {0, 0, 0}, 1);
			TS_ASSERT_DELTA(result.movement.x, 1, 1e-9);
			TS_ASSERT_DELTA(result.movement.y, 3, 1e-9);
			TS_ASSERT_DELTA(result.velocity.y, 3, 1e-9);
		}

		void testSimultaneousCornerContactCannotEnterBlock() {
			std::vector<collision::Obstacle> walls{{DoubleBox{2, 2, 0, 1, 1, 3}}};
			auto result = collision::sweep(walls, {0, 0, 0, 1, 1, 1}, {4, 4, 0}, {0, 0, 0}, 1);
			DoubleBox finalBox{0, 0, 0, 1, 1, 1};
			finalBox.move(result.movement);
			TS_ASSERT(!finalBox.intersects(walls.front().box));
			TS_ASSERT(result.collides);
		}

		void testGroundContactDoesNotBlockTangentMotion() {
			std::vector<collision::Obstacle> floor{{DoubleBox{-10, -10, -1, 20, 20, 1}, 0, 3}};
			auto result = collision::sweep(floor, {0, 0, 0, 0.6, 0.6, 1.8}, {2, 0, 0}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(result.movement.x, 0.2, 1e-9);
			TS_ASSERT(result.grounded);
			TS_ASSERT_EQUALS(result.slippery, 3);
			TS_ASSERT(!result.collides);
		}

		void testFloorAndCeilingStopVerticalMotion() {
			std::vector<collision::Obstacle> floor{{DoubleBox{-10, -10, -1, 20, 20, 1}}};
			auto down = collision::sweep(floor, {0, 0, 1, 0.6, 0.6, 1.8}, {0, 0, -20}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(down.movement.z, -1, 1e-9);
			TS_ASSERT(down.grounded);
			std::vector<collision::Obstacle> ceiling{{DoubleBox{-10, -10, 3, 20, 20, 1}}};
			auto up = collision::sweep(ceiling, {0, 0, 0, 0.6, 0.6, 1.8}, {0, 0, 20}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(up.movement.z, 1.2, 1e-9);
			TS_ASSERT_DELTA(up.velocity.z, 0, 1e-9);
			TS_ASSERT(!up.grounded);
		}

		void testShallowOverlapRecoversAgainstVelocity() {
			std::vector<collision::Obstacle> floor{{DoubleBox{-10, -10, -1, 20, 20, 1}}};
			auto result = collision::sweep(floor, {0, 0, -0.05, 0.6, 0.6, 1.8}, {1, 0, -1}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(result.movement.z, 0.05, 1e-9);
			TS_ASSERT_DELTA(result.movement.x, 0.1, 1e-9);
			TS_ASSERT(result.grounded);
		}

		void testStepAndCeilingClearance() {
			std::vector<collision::Obstacle> boxes{
				{DoubleBox{-10, -10, -1, 20, 20, 1}},
				{DoubleBox{1, -1, 0, 1, 3, 0.5}}
			};
			DoubleBox body{0, 0, 0, 0.6, 0.6, 1.8};
			auto step = collision::sweep(boxes, body, {2, 0, 0}, {0, 0, 0}, 0.5, 0.6);
			TS_ASSERT_DELTA(step.movement.x, 1, 1e-9);
			TS_ASSERT_DELTA(step.movement.z, 0.5, 1e-9);
			TS_ASSERT(step.stepped && step.grounded);
			boxes.push_back({DoubleBox{-1, -1, 2, 4, 4, 1}});
			auto blocked = collision::sweep(boxes, body, {2, 0, 0}, {0, 0, 0}, 0.5, 0.6);
			TS_ASSERT_DELTA(blocked.movement.x, 0.4, 1e-9);
			TS_ASSERT_DELTA(blocked.movement.z, 0, 1e-9);
			TS_ASSERT(!blocked.stepped);
		}

		void testLongStepCannotLiftIntoLaterCeiling() {
			std::vector<collision::Obstacle> boxes{
				{DoubleBox{-10, -10, -1, 20, 20, 1}},
				{DoubleBox{1, -1, 0, 4, 3, 0.5}},
				{DoubleBox{2, -1, 2.1, 4, 3, 1}}
			};
			auto result = collision::sweep(boxes, {0, 0, 0, 0.6, 0.6, 1.8}, {3, 0, 0}, {0, 0, 0}, 1, 0.6);
			DoubleBox finalBox{0, 0, 0, 0.6, 0.6, 1.8};
			finalBox.move(result.movement);
			finalBox.move(1e-9, 1e-9, 1e-9);
			finalBox.sizeX -= 2e-9;
			finalBox.sizeY -= 2e-9;
			finalBox.sizeZ -= 2e-9;
			for (const auto &obstacle : boxes)
				TS_ASSERT(!finalBox.intersects(obstacle.box));
			TS_ASSERT(!result.stepped);
		}

		void testUnloadedBoundaryDoesNotBecomeAStep() {
			std::vector<collision::Obstacle> boxes{{DoubleBox{1, -1, 0, 1, 3, 0.5}, 0, 0, true}};
			auto result = collision::sweep(boxes, {0, 0, 0, 0.6, 0.6, 1.8}, {2, 0, 0}, {0, 0, 0}, 0.5, 0.6);
			TS_ASSERT_DELTA(result.movement.x, 0.4, 1e-9);
			TS_ASSERT(result.unloaded && !result.stepped);
		}

		void testBounceThresholdAndRemainingTime() {
			std::vector<collision::Obstacle> floor{{DoubleBox{-10, -10, -1, 20, 20, 1}, -80, 0}};
			auto fast = collision::sweep(floor, {0, 0, 1, 0.6, 0.6, 1.8}, {0, 0, -10}, {0, 0, 0}, 0.2);
			TS_ASSERT_DELTA(fast.velocity.z, 8, 1e-9);
			TS_ASSERT_DELTA(fast.movement.z, -0.2, 1e-9);
			TS_ASSERT_EQUALS(fast.bouncy, -80);
			auto slow = collision::sweep(floor, {0, 0, 0, 0.6, 0.6, 1.8}, {0, 0, -3}, {0, 0, 0}, 0.1);
			TS_ASSERT_DELTA(slow.velocity.z, 0, 1e-9);
			TS_ASSERT(slow.grounded);
		}

		void testAccelerationAndSourceSlipFactor() {
			auto result = collision::sweep({}, {0, 0, 2, 0.6, 0.6, 1.8}, {2, 0, 0}, {0, 0, -10}, 0.1);
			TS_ASSERT_DELTA(result.movement.x, 0.2, 1e-9);
			TS_ASSERT_DELTA(result.movement.z, -0.05, 1e-9);
			TS_ASSERT_DELTA(result.velocity.z, -1, 1e-9);
			TS_ASSERT_DELTA(collision::friction(3, true), 0.25, 1e-9);
			TS_ASSERT_DELTA(collision::friction(3, false), 1.0 / 7, 1e-9);
			TS_ASSERT_EQUALS(collision::friction(0, false), 1);
		}
};

#endif // LUNARCHYCOLLISIONTESTS_HPP_
