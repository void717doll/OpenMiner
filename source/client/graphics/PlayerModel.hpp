#ifndef PLAYERMODEL_HPP_
#define PLAYERMODEL_HPP_

#include <array>
#include "client/graphics/Drawable.hpp"
#include "client/graphics/Texture.hpp"
#include "client/graphics/VertexBuffer.hpp"

class Player;

class PlayerModel : public Drawable {
	public:
		explicit PlayerModel(const Player &player);
	private:
		void draw(RenderTarget &target, RenderStates states) const override;
		const Player &m_player;
		Texture m_skin;
		mutable std::array<Texture, 4> m_armor;
		std::array<VertexBuffer, 5> m_vertices;
		std::array<u32, 5> m_counts{};
};

#endif // PLAYERMODEL_HPP_
