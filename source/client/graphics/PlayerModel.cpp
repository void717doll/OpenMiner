#include "common/scene/RetroMobMesh.hpp"
#include "common/world/Player.hpp"
#include "client/graphics/PlayerModel.hpp"
#include "client/graphics/RenderTarget.hpp"
#include "client/graphics/Vertex.hpp"

PlayerModel::PlayerModel(const Player &player) : m_player(player), m_skin("resources/textures/player.png") {
	for (unsigned layer = 0; layer < 5; ++layer) {
		retro::MobMeshBuilder mesh;
		float inflate = layer == 0 ? 0 : layer == 3 ? 0.5f : 1.f;
		if (layer == 0 || layer == 1) mesh.box(-4,-8,-4,8,8,8,0,0,0,layer == 1 ? 32.f : 0.f,0,0,0,0,inflate);
		if (layer == 0 || layer == 2 || layer == 3) mesh.box(-4,0,-2,8,12,4,0,0,0,16,16,0,0,0,inflate);
		if (layer == 0 || layer == 2) {
			mesh.box(-3,-2,-2,4,12,4,-5,2,0,40,16,0,0,0.07f,inflate);
			mesh.box(-1,-2,-2,4,12,4,5,2,0,40,16,0,0,-0.07f,inflate,true);
		}
		if (layer == 0 || layer == 3 || layer == 4) {
			mesh.box(-2,0,-2,4,12,4,-2,12,0,0,16,0,0,0,inflate);
			mesh.box(-2,0,-2,4,12,4,2,12,0,0,16,0,0,0,inflate,true);
		}
		std::vector<Vertex> vertices;
		for (const auto &point : mesh.vertices) {
			Vertex vertex;
			vertex.coord3d[0] = point.x; vertex.coord3d[1] = point.y; vertex.coord3d[2] = point.z;
			vertex.texCoord[0] = point.u; vertex.texCoord[1] = point.v;
			for (float &value : vertex.color) value = 1;
			vertices.push_back(vertex);
		}
		m_counts[layer] = u32(vertices.size());
		m_vertices[layer].setupDefaultLayout();
		m_vertices[layer].init(vertices.data(), u32(vertices.size() * sizeof(Vertex)));
	}
}

void PlayerModel::draw(RenderTarget &target, RenderStates states) const {
	states.isDepthTestEnabled = true;
	states.isCullFaceEnabled = false;
	states.texture = &m_skin;
	target.draw(m_vertices[0], 0, m_counts[0], states);
	for (u16 slot = 0; slot < 4; ++slot) {
		const auto &stack = m_player.armor().getStack(0, slot);
		if (!stack.amount()) continue;
		std::string id = stack.stringID();
		std::string path = "mods/" + stack.item().modName() + "/textures/armor/" + id.substr(id.find(':') + 1) + ".png";
		if (m_armor[slot].filename() != path) {
			m_armor[slot].free();
			m_armor[slot].loadFromFile(path);
		}
		states.texture = &m_armor[slot];
		target.draw(m_vertices[slot + 1], 0, m_counts[slot + 1], states);
	}
}
