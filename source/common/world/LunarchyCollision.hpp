// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// adapted from lunarchy/src/collision.cpp: z-up coordinates, block units and double precision.

#ifndef LUNARCHYCOLLISION_HPP_
#define LUNARCHYCOLLISION_HPP_

#include <array>
#include <cmath>
#include <vector>

#include "common/core/Box.hpp"

namespace collision {

struct Obstacle {
	DoubleBox box;
	int bouncy = 0;
	int slippery = 0;
	bool unloaded = false;
	bool stepped = false;
	Vector3i node{0, 0, 0};
	bool object = false;
};

struct Result {
	Vector3d movement{0, 0, 0};
	Vector3d velocity{0, 0, 0};
	bool grounded = false;
	bool collides = false;
	bool horizontalCollision = false;
	bool stepped = false;
	bool unloaded = false;
	int slippery = 0;
	int bouncy = 0;
	Vector3i supportNode{0, 0, 0};
	bool hasSupport = false;
	bool standingOnObject = false;
};

inline double &component(Vector3d &vector, int axis) {
	return axis == 0 ? vector.x : axis == 1 ? vector.y : vector.z;
}

inline double minimum(const DoubleBox &box, int axis) {
	return axis == 0 ? box.x : axis == 1 ? box.y : box.z;
}

inline double extent(const DoubleBox &box, int axis) {
	return axis == 0 ? box.sizeX : axis == 1 ? box.sizeY : box.sizeZ;
}

inline double maximum(const DoubleBox &box, int axis) {
	return minimum(box, axis) + extent(box, axis);
}

inline Vector3d limitVelocity(Vector3d velocity, bool truncate = true) {
	for (int axis = 0; axis < 3; ++axis) {
		double &value = component(velocity, axis);
		value = std::clamp(value, -500.0, 500.0);
		if (truncate)
			value = std::trunc(value * 100000.0) / 100000.0;
	}
	return velocity;
}

inline int axisAlignedCollision(const DoubleBox &obstacle, const DoubleBox &moving,
	Vector3d velocity, double &time) {
	const double limit = time;
	// the source checks its vertical axis before the horizontal axes.
	for (int axis : {2, 0, 1}) {
		double speed = component(velocity, axis);
		if (speed == 0)
			continue;

		double distance = std::max(maximum(moving, axis), maximum(obstacle, axis))
			- std::min(minimum(moving, axis), minimum(obstacle, axis))
			- extent(moving, axis) - extent(obstacle, axis);
		double contactTime = distance / std::abs(speed);
		if (contactTime > limit)
			return -1;

		double innerMargin = std::max(-0.5 * extent(obstacle, axis), -0.2);
		bool approaching = speed > 0
			? minimum(obstacle, axis) - maximum(moving, axis) > innerMargin
			: minimum(moving, axis) - maximum(obstacle, axis) > innerMargin;
		if (!approaching)
			continue;

		bool overlaps = true;
		for (int other = 0; other < 3; ++other) {
			if (other == axis)
				continue;
			double displacement = component(velocity, other) * std::max(contactTime, 0.0);
			double low = minimum(moving, other) + displacement;
			double high = maximum(moving, other) + displacement;
			double overlap = std::min(high, maximum(obstacle, other)) - std::max(low, minimum(obstacle, other));
			if (overlap > 1e-10)
				continue;
			// simultaneous corner contacts must stop entry, while tangent motion stays free.
			bool entering = (component(velocity, other) > 0 && std::abs(high - minimum(obstacle, other)) < 1e-10)
				|| (component(velocity, other) < 0 && std::abs(low - maximum(obstacle, other)) < 1e-10);
			if (overlap < -1e-10 || !entering) {
				overlaps = false;
				break;
			}
		}
		if (overlaps) {
			time = contactTime;
			return axis;
		}
	}
	return -1;
}

inline bool wouldCollideWithCeiling(const std::vector<Obstacle> &obstacles,
	const DoubleBox &box, double rise) {
	for (const auto &obstacle : obstacles) {
		const auto &other = obstacle.box;
		if (box.z + box.sizeZ <= other.z && box.z + box.sizeZ + rise > other.z
		 && box.x < other.x + other.sizeX && box.x + box.sizeX > other.x
		 && box.y < other.y + other.sizeY && box.y + box.sizeY > other.y)
			return true;
	}
	return false;
}

inline double friction(int slippery, bool hasMovementInput) {
	if (slippery <= 0)
		return 1;
	if (!hasMovementInput)
		slippery *= 2;
	return std::clamp(1.0 / (slippery + 1), 0.001, 1.0);
}

inline Result sweep(std::vector<Obstacle> obstacles, DoubleBox box, Vector3d velocity,
	Vector3d acceleration, double seconds, double stepHeight = 0, bool bounce = true, bool truncateVelocity = true) {
	Result result;
	result.velocity = velocity;
	const DoubleBox startBox = box;
	const Vector3d startVelocity = velocity;
	const Vector3d startAcceleration = acceleration;
	const double startSeconds = seconds;
	seconds = std::clamp(seconds, 0.0, 2.5);
	Vector3d average = limitVelocity(velocity + acceleration * (0.5 * seconds), truncateVelocity);
	const Vector3d start = box.position();

	for (int iteration = 0; iteration < 100 && seconds > 1e-9; ++iteration) {
		double nearestTime = seconds;
		int nearestAxis = -1;
		int nearestIndex = -1;
		for (std::size_t i = 0; i < obstacles.size(); ++i) {
			if (obstacles[i].stepped)
				continue;
			double time = nearestTime;
			int axis = axisAlignedCollision(obstacles[i].box, box, average, time);
			if (axis < 0 || time > nearestTime)
				continue;
			nearestTime = time;
			nearestAxis = axis;
			nearestIndex = int(i);
		}

		if (nearestIndex < 0) {
			box.move(average * seconds);
			velocity = limitVelocity(velocity + acceleration * seconds, truncateVelocity);
			break;
		}

		auto &obstacle = obstacles[nearestIndex];
		const auto &other = obstacle.box;
		bool stepUp = false;
		if (nearestAxis != 2 && !obstacle.unloaded) {
			DoubleBox stepBox = box;
			double lookAhead = nearestTime + 0.1 * std::abs(seconds - nearestTime);
			stepBox.move(average.x * lookAhead, average.y * lookAhead, 0);
			double rise = other.z + other.sizeZ - box.z;
			stepUp = rise > 0 && rise < stepHeight
				&& !wouldCollideWithCeiling(obstacles, stepBox, rise);
		}

		if (nearestTime < 0) {
			if (!stepUp) {
				Vector3d recovery{0, 0, 0};
				component(recovery, nearestAxis) = component(average, nearestAxis) * nearestTime;
				box.move(recovery);
			}
		}
		else if (nearestTime > 0) {
			average = velocity + acceleration * (0.5 * nearestTime);
			box.move(average * nearestTime);
			velocity = limitVelocity(velocity + acceleration * nearestTime, truncateVelocity);
			seconds -= nearestTime;
		}

		if (stepUp) {
			obstacle.stepped = true;
		}
		else {
			result.collides = true;
			result.unloaded |= obstacle.unloaded;
			result.horizontalCollision |= nearestAxis != 2;
			double &speed = component(velocity, nearestAxis);
			if (nearestAxis == 2 && speed < 0) {
				result.bouncy = obstacle.bouncy;
				result.slippery = obstacle.slippery;
				result.supportNode = obstacle.node;
				result.hasSupport = !obstacle.unloaded && !obstacle.object;
				result.standingOnObject = obstacle.object;
			}
			if (bounce && obstacle.bouncy && std::abs(speed) > 3)
				speed *= -std::abs(obstacle.bouncy) / 100.0;
			else {
				if (nearestAxis == 2 && speed < 0)
					result.grounded = true;
				speed = 0;
				component(acceleration, nearestAxis) = 0;
			}
		}
		average = limitVelocity(velocity + acceleration * (0.5 * seconds), truncateVelocity);
	}

	for (const auto &obstacle : obstacles) {
		const auto &other = obstacle.box;
		if (other.x + other.sizeX <= box.x || other.x >= box.x + box.sizeX
		 || other.y + other.sizeY <= box.y || other.y >= box.y + box.sizeY)
			continue;
		if (obstacle.stepped) {
			double rise = other.z + other.sizeZ - box.z;
			// long steps can reach a ceiling beyond the source's short lookahead.
			if (rise > 0 && wouldCollideWithCeiling(obstacles, box, rise)) {
				for (auto &candidate : obstacles)
					candidate.stepped = false;
				return sweep(std::move(obstacles), startBox, startVelocity, startAcceleration,
					startSeconds, 0, bounce, truncateVelocity);
			}
			box.z += rise;
			result.stepped = true;
		}
		if (std::abs(other.z + other.sizeZ - box.z) < 0.005) {
			result.grounded = true;
			result.unloaded |= obstacle.unloaded;
			result.slippery = obstacle.slippery;
			result.bouncy = obstacle.bouncy;
			result.supportNode = obstacle.node;
			result.hasSupport = !obstacle.unloaded && !obstacle.object;
			result.standingOnObject = obstacle.object;
		}
	}

	result.movement = box.position() - start;
	result.velocity = velocity;
	return result;
}

}

#endif // LUNARCHYCOLLISION_HPP_
