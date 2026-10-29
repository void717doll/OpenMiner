#ifndef RETRORANDOM_HPP_
#define RETRORANDOM_HPP_

#include <cstdint>
#include <stdexcept>

namespace retro {

// java.util.Random's 48-bit generator, used by the supplied beta sources.
class Random {
	public:
		explicit Random(std::int64_t seed) { setSeed(seed); }
		void setSeed(std::int64_t seed) { m_seed = (std::uint64_t(seed) ^ 0x5deece66dULL) & mask; }
		float nextFloat() { return float(next(24)) / 16777216.0f; }
		std::int64_t nextLong() {
			std::int32_t high = std::int32_t(next(32));
			std::int32_t low = std::int32_t(next(32));
			return std::int64_t((std::uint64_t(std::int64_t(high)) << 32) + std::uint64_t(std::int64_t(low)));
		}
		std::uint32_t next(unsigned bits) {
			m_seed = (m_seed * 0x5deece66dULL + 11) & mask;
			return std::uint32_t(m_seed >> (48 - bits));
		}
		int nextInt(int bound) {
			if (bound <= 0) throw std::invalid_argument("random bound must be positive");
			if ((bound & -bound) == bound)
				return int((std::uint64_t(bound) * next(31)) >> 31);
			std::uint32_t bits, value;
			do {
				bits = next(31);
				value = bits % unsigned(bound);
			} while (bits - value + unsigned(bound - 1) >= 0x80000000u);
			return int(value);
		}
		double nextDouble() {
			std::uint64_t high = next(26);
			return double((high << 27) + next(27)) / 9007199254740992.0;
		}
	private:
		static constexpr std::uint64_t mask = (1ULL << 48) - 1;
		std::uint64_t m_seed;
};

}

#endif // RETRORANDOM_HPP_
