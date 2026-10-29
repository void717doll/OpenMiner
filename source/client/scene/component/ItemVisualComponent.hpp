#ifndef ITEMVISUALCOMPONENT_HPP_
#define ITEMVISUALCOMPONENT_HPP_

#include <algorithm>
#include "common/core/IntTypes.hpp"
#include "common/core/Vector3.hpp"

struct ItemVisualComponent {
	Vector3d previous;
	Vector3d target;
	u64 updated = 0;
	bool initialized = false;

	Vector3d sample(u64 now) const {
		double fraction = std::min(double(now - updated) / 50.0, 1.0);
		return previous + (target - previous) * fraction;
	}

	Vector3d update(const Vector3d &position, u64 now) {
		if (!initialized || (position - target).length() > 4) {
			previous = target = position;
			updated = now;
			initialized = true;
		}
		else if (!(position - target).isZero()) {
			previous = sample(now);
			target = position;
			updated = now;
		}
		return sample(now);
	}
};

#endif // ITEMVISUALCOMPONENT_HPP_
