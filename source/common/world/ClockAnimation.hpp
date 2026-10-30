#ifndef CLOCKANIMATION_HPP_
#define CLOCKANIMATION_HPP_

#include <algorithm>
#include <cmath>
#include "common/core/IntTypes.hpp"

class ClockAnimation {
	public:
		unsigned update(u64 ticks, u16 dimension) {
			constexpr double tau = 6.283185307179586;
			if (ticks != m_lastTick || dimension != m_dimension) {
				u64 steps = m_lastTick == ~u64(0) ? 1 : std::min<u64>(20, ticks > m_lastTick ? ticks - m_lastTick : 1);
				for (u64 step = 0; step < steps; ++step) {
					double target = (double(ticks % 24000) / 24000.0 - 0.25) * tau;
					if (dimension) {
						m_random = m_random * 1664525u + 1013904223u;
						target = double(m_random) / 4294967296.0 * tau;
					}
					if (m_lastTick == ~u64(0) && !dimension) m_angle = target;
					double difference = std::remainder(target - m_angle, tau);
					m_velocity = (m_velocity + std::clamp(difference, -1.0, 1.0) * 0.1) * 0.8;
					m_angle = std::remainder(m_angle + m_velocity, tau);
				}
				m_lastTick = ticks;
				m_dimension = dimension;
			}
			return unsigned(int(std::floor(m_angle / tau * 64 + 0.5)) & 63);
		}

	private:
		u64 m_lastTick = ~u64(0);
		u16 m_dimension = 0;
		u32 m_random = 0x9e3779b9;
		double m_angle = 0;
		double m_velocity = 0;
};

#endif // CLOCKANIMATION_HPP_
