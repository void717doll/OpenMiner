#ifndef ITEMBILLBOARD_HPP_
#define ITEMBILLBOARD_HPP_

#include "client/graphics/Drawable.hpp"
#include "client/graphics/VertexBuffer.hpp"
#include "client/graphics/TextureAtlas.hpp"
#include "client/graphics/Vertex.hpp"
#include <vector>

class ItemBillboard : public Drawable {
	public:
		ItemBillboard(const std::string &item, float size);
		void setLight(u8 sunlight, u8 torchlight);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		TextureAtlas &m_atlas;
		std::vector<Vertex> m_mesh;
		u8 m_sunlight = 15;
		u8 m_torchlight = 0;
		VertexBuffer m_vertices;
};

#endif // ITEMBILLBOARD_HPP_
