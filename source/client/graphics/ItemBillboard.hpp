#ifndef ITEMBILLBOARD_HPP_
#define ITEMBILLBOARD_HPP_

#include "client/graphics/Drawable.hpp"
#include "client/graphics/VertexBuffer.hpp"
#include "client/graphics/TextureAtlas.hpp"

class ItemBillboard : public Drawable {
	public:
		ItemBillboard(const std::string &item, float size);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		TextureAtlas &m_atlas;
		FloatRect m_uv;
		float m_size;
		mutable VertexBuffer m_vertices;
};

#endif // ITEMBILLBOARD_HPP_
