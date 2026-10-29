#ifndef MOBMODEL_HPP_
#define MOBMODEL_HPP_

#include <array>

#include "common/core/Vector3.hpp"
#include "client/graphics/Drawable.hpp"
#include "client/graphics/Texture.hpp"
#include "client/graphics/VertexBuffer.hpp"

class MobModel : public Drawable {
	public:
		explicit MobModel(unsigned model, float scale = 1.f, u8 appearance = 0);
		void configure(unsigned model, float scale, u8 appearance);
		void updateMotion(const Vector3d &position);
		bool hasCollision() const { return m_model > 0 && m_model < 15; }
		bool isTargetable() const { return m_model != 15; }
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		unsigned m_model = 0;
		float m_scale = 1;
		u8 m_appearance = 0;
		Texture m_texture;
		Texture m_overlay;
		mutable std::array<VertexBuffer, 2> m_vertices;
		Vector3d m_lastPosition;
		u64 m_lastMove = 0;
		float m_stride = 0;
		bool m_hasPosition = false;
};

#endif // MOBMODEL_HPP_
