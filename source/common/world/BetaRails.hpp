#ifndef BETARAILS_HPP_
#define BETARAILS_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include "common/core/Vector3.hpp"

namespace retro {

// entity minecart and rail logic endpoints, converted from y-up to z-up.
inline std::array<Vector3d, 2> railEnds(unsigned shape) {
	static const std::array<std::array<Vector3d, 2>, 10> ends{{
		{{{0.5,0,0},{0.5,1,0}}}, {{{0,0.5,0},{1,0.5,0}}},
		{{{0,0.5,0},{1,0.5,1}}}, {{{0,0.5,1},{1,0.5,0}}},
		{{{0.5,0,1},{0.5,1,0}}}, {{{0.5,0,0},{0.5,1,1}}},
		{{{0.5,1,0},{1,0.5,0}}}, {{{0.5,1,0},{0,0.5,0}}},
		{{{0.5,0,0},{0,0.5,0}}}, {{{0.5,0,0},{1,0.5,0}}}
	}};
	return ends[shape < ends.size() ? shape : 0];
}

inline Vector3d railPoint(unsigned shape, Vector3d local) {
	auto ends = railEnds(shape);
	Vector3d axis = ends[1] - ends[0];
	double length = axis.x * axis.x + axis.y * axis.y;
	double distance = ((local.x - ends[0].x) * axis.x + (local.y - ends[0].y) * axis.y) / length;
	return ends[0] + axis * distance;
}

// neighbor heights use -2 for absent; order is north, south, west, east.
inline unsigned railShape(const std::array<int, 4> &heights, bool curves = true, bool powered = false) {
	bool n = heights[0] != -2, s = heights[1] != -2;
	bool w = heights[2] != -2, e = heights[3] != -2;
	int shape = -1;
	if ((n || s) && !w && !e) shape = 0;
	if ((w || e) && !n && !s) shape = 1;
	if (curves) {
		if (s && e && !n && !w) shape = 6;
		if (s && w && !n && !e) shape = 7;
		if (n && w && !s && !e) shape = 8;
		if (n && e && !s && !w) shape = 9;
	}
	if (shape < 0) {
		if (n || s) shape = 0;
		if (w || e) shape = 1;
		// modernmcp changes the selected branch when a junction receives power.
		if (curves && powered) {
			if (s && e) shape = 6;
			if (s && w) shape = 7;
			if (n && e) shape = 9;
			if (n && w) shape = 8;
		} else if (curves) {
			if (n && w) shape = 8;
			if (n && e) shape = 9;
			if (s && w) shape = 7;
			if (s && e) shape = 6;
		}
	}
	if (shape == 0) {
		if (heights[0] == 1) shape = 4;
		if (heights[1] == 1) shape = 5;
	}
	if (shape == 1) {
		if (heights[3] == 1) shape = 2;
		if (heights[2] == 1) shape = 3;
	}
	return shape < 0 ? 0 : unsigned(shape);
}

inline void alignCartVelocity(Vector3d &velocity, unsigned shape, bool gravity = true) {
	if (gravity) {
		if (shape == 2) velocity.x -= 1.0 / 128;
		if (shape == 3) velocity.x += 1.0 / 128;
		if (shape == 4) velocity.y += 1.0 / 128;
		if (shape == 5) velocity.y -= 1.0 / 128;
	}
	auto ends = railEnds(shape);
	Vector3d direction = ends[1] - ends[0];
	double length = std::hypot(direction.x, direction.y);
	double speed = std::min(2.0, std::hypot(velocity.x, velocity.y));
	if (velocity.x * direction.x + velocity.y * direction.y < 0) speed = -speed;
	velocity = {direction.x / length * speed, direction.y / length * speed, 0};
}

}
#endif
