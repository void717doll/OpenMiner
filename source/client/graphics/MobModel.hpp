#ifndef MOBMODEL_HPP_
#define MOBMODEL_HPP_

#include "common/core/Vector3.hpp"
#include "client/graphics/Drawable.hpp"
#include "client/graphics/Texture.hpp"
#include "client/graphics/VertexBuffer.hpp"

class MobModel : public Drawable {
	public:
		explicit MobModel(unsigned model);
		void updateMotion(const Vector3d &position);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		unsigned m_model;
		Texture m_texture;
		mutable VertexBuffer m_vertices;
		Vector3d m_lastPosition;
		u64 m_lastMove = 0;
		bool m_hasPosition = false;
};

#endif // MOBMODEL_HPP_
