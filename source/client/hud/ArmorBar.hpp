#ifndef ARMORBAR_HPP_
#define ARMORBAR_HPP_

#include <array>

#include "client/graphics/Image.hpp"

class ArmorBar : public Drawable, public Transformable {
	public:
		ArmorBar();
		void update(unsigned protection);

	private:
		void draw(RenderTarget &target, RenderStates states) const override;

		std::array<Image, 10> m_empty;
		std::array<Image, 10> m_filled;
		unsigned m_protection = 0;
};

#endif // ARMORBAR_HPP_
