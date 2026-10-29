#ifndef HEARTBAR_HPP_
#define HEARTBAR_HPP_

#include <array>

#include "common/core/HeartBarState.hpp"
#include "client/graphics/Image.hpp"

class HeartBar : public Drawable, public Transformable {
	public:
		HeartBar();
		void update(unsigned health, u32 damageSerial = 0);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		std::array<Image, hearts::count> m_empty;
		std::array<Image, hearts::count> m_filled;
		std::array<hearts::Fill, hearts::count> m_states{};
		unsigned m_health = 21;
		unsigned m_displayedHealth = 21;
		unsigned m_previousHealth = 20;
		u32 m_damageSerial = 0;
		u32 m_flashUntil = 0;
};

#endif // HEARTBAR_HPP_
