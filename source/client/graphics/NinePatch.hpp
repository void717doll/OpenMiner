#ifndef NINEPATCH_HPP_
#define NINEPATCH_HPP_

#include <array>
#include "client/graphics/Image.hpp"

class NinePatch : public Drawable, public Transformable {
	public:
		explicit NinePatch(const std::string &texture, u16 border = 2);
		explicit NinePatch(const Texture &texture, u16 border = 2);
		void setSize(u16 width, u16 height);
		void setColor(const Color &color);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		std::array<Image, 9> m_parts;
		u16 m_sourceWidth, m_sourceHeight, m_border;
};

#endif // NINEPATCH_HPP_
