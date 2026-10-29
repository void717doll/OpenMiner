// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>
// adapted from lunarchy/src/client/localplayer.cpp and clientenvironment.cpp.

#ifndef SURVIVALMOVEMENT_HPP_
#define SURVIVALMOVEMENT_HPP_

#include <algorithm>
#include <cmath>

#include "common/core/Vector3.hpp"

namespace survival {

// lunarchy uses BS=10; these values are converted to blocks and seconds.
constexpr double walkSpeed = 4;
constexpr double crouchSpeed = 1.35;
constexpr double climbSpeed = 3;
constexpr double jumpSpeed = 6.5;
constexpr double groundAcceleration = 30;
constexpr double airAcceleration = 20;
constexpr double gravity = 19.62;
constexpr double liquidSink = 20;

inline void accelerate(Vector3d &velocity, const Vector3d &target, double horizontal, double vertical) {
	double dx = target.x - velocity.x, dy = target.y - velocity.y;
	double distance = std::hypot(dx,dy);
	if (horizontal > 0 && distance > 0) {
		double factor = std::min(horizontal / distance,1.0);
		velocity.x += dx * factor;
		velocity.y += dy * factor;
	}
	if (vertical > 0) velocity.z += std::clamp(target.z - velocity.z,-vertical,vertical);
}

inline void applyResistance(Vector3d &velocity, unsigned resistance, double seconds, bool inLiquid) {
	if (!resistance) return;
	double length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
	if (length == 0) return;
	double reduction = inLiquid ? std::min(length,5.0) : length;
	reduction *= (double(resistance) * 0.3 + 0.7) * seconds * 10;
	velocity *= 1 - reduction / length;
}

inline double bounceBoost(double incomingVerticalSpeed, bool jump) {
	// LocalPlayer::move uses the incoming speed here, before collision rebound.
	return jump ? jumpSpeed / (1 + incomingVerticalSpeed * 2.8 / jumpSpeed) : -incomingVerticalSpeed / 3;
}

}

#endif // SURVIVALMOVEMENT_HPP_
