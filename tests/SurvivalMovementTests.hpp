#ifndef SURVIVALMOVEMENTTESTS_HPP_
#define SURVIVALMOVEMENTTESTS_HPP_

#include <cxxtest/TestSuite.h>

#include "common/world/SurvivalMovement.hpp"
#include "common/world/LunarchyCollision.hpp"

class SurvivalMovementTests : public CxxTest::TestSuite {
	public:
		void testHorizontalAccelerationKeepsDirectionAndCapsIncrement() {
			Vector3d speed{0,0,-5};
			survival::accelerate(speed,{3,4,0},1,0);
			TS_ASSERT_DELTA(speed.x,0.6,1e-12);
			TS_ASSERT_DELTA(speed.y,0.8,1e-12);
			TS_ASSERT_EQUALS(speed.z,-5);
			survival::accelerate(speed,{3,4,2},10,0.25);
			TS_ASSERT_EQUALS(speed.x,3);
			TS_ASSERT_EQUALS(speed.y,4);
			TS_ASSERT_EQUALS(speed.z,-4.75);
		}

		void testWalkingAndBrakingAtDifferentStepSizes() {
			for (double seconds : {0.006,0.01}) {
				Vector3d speed{0,0,0};
				for (int step = 0; step < 30; ++step)
					survival::accelerate(speed,{survival::walkSpeed,0,0},survival::groundAcceleration * seconds,0);
				TS_ASSERT_DELTA(speed.x,4,1e-12);
				for (int step = 0; step < 30; ++step)
					survival::accelerate(speed,{0,0,0},survival::groundAcceleration * seconds,0);
				TS_ASSERT_DELTA(speed.x,0,1e-12);
			}
		}

		void testWaterAndLavaResistanceUsesSecondsAndWholeSpeed() {
			Vector3d water{3,0,-4};
			survival::applyResistance(water,1,0.01,true);
			TS_ASSERT_DELTA(water.x,2.7,1e-12);
			TS_ASSERT_DELTA(water.z,-3.6,1e-12);
			Vector3d lava{3,0,-4};
			survival::applyResistance(lava,7,0.01,true);
			TS_ASSERT_DELTA(lava.x,2.16,1e-12);
			TS_ASSERT_DELTA(lava.z,-2.88,1e-12);
			Vector3d fast{10,0,0};
			survival::applyResistance(fast,1,0.01,true);
			TS_ASSERT_DELTA(fast.x,9.5,1e-12);
		}

		void testSourceBounceControlUsesIncomingVelocity() {
			TS_ASSERT_DELTA(survival::bounceBoost(-12,false),4,1e-12);
			TS_ASSERT_DELTA(survival::bounceBoost(2,true),6.5 / (1 + 2 * 2.8 / 6.5),1e-12);
		}

		void testJumpArcAndLandingAtClientStepSizes() {
			std::vector<collision::Obstacle> floor{{DoubleBox{-10,-10,-1,20,20,1}}};
			for (double seconds : {0.006,0.01}) {
				DoubleBox body{-0.3,-0.3,0,0.6,0.6,1.77};
				Vector3d velocity{0,0,survival::jumpSpeed};
				double peak = 0;
				bool landed = false;
				for (double time = 0; time < 1; time += seconds) {
					auto result = collision::sweep(floor,body,velocity,{0,0,-survival::gravity},seconds);
					body.move(result.movement);
					velocity = result.velocity;
					peak = std::max(peak,body.z);
					landed |= result.grounded;
				}
				TS_ASSERT(peak > 1.07 && peak < 1.08);
				TS_ASSERT(landed);
				TS_ASSERT_DELTA(body.z,0,1e-4);
			}
		}
};

#endif // SURVIVALMOVEMENTTESTS_HPP_
