#ifndef HEARTBAR_HPP_
#define HEARTBAR_HPP_

#include <array>

#include "common/core/HeartBarState.hpp"
#include "client/graphics/Image.hpp"

class HeartBar : public Drawable, public Transformable {
	public:
		HeartBar();
		void update(unsigned health);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		std::array<Image, hearts::count> m_empty;
		std::array<Image, hearts::count> m_filled;
		std::array<hearts::Fill, hearts::count> m_states{};
		unsigned m_health = 21;
};

#endif // HEARTBAR_HPP_
