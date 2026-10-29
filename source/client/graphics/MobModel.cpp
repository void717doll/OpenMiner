#include "common/core/GameClock.hpp"
#include "common/scene/RetroMobMesh.hpp"
#include "client/graphics/MobModel.hpp"
#include "client/graphics/RenderTarget.hpp"
#include "client/graphics/Vertex.hpp"

MobModel::MobModel(unsigned model) : m_model(model) {
	m_texture.loadFromFile(model == 1 ? "mods/default/textures/mobs/pig.png" : "mods/default/textures/mobs/zombie.png");
	m_vertices.setupDefaultLayout();
}

void MobModel::updateMotion(const Vector3d &position) {
	if (m_hasPosition && (std::abs(position.x - m_lastPosition.x) > 0.001 || std::abs(position.y - m_lastPosition.y) > 0.001))
		m_lastMove = GameClock::getInstance().getTicks();
	m_lastPosition = position;
	m_hasPosition = true;
}

void MobModel::draw(RenderTarget &target, RenderStates states) const {
	u64 now = GameClock::getInstance().getTicks();
	auto mesh = retro::mobMesh(m_model, float(now % 100000) * 0.008f, now - m_lastMove < 200 ? 0.6f : 0.f);
	std::vector<Vertex> vertices;
	vertices.reserve(mesh.size());
	for (const auto &point : mesh) {
		Vertex vertex;
		vertex.coord3d[0] = point.x; vertex.coord3d[1] = point.y; vertex.coord3d[2] = point.z;
		vertex.texCoord[0] = point.u; vertex.texCoord[1] = point.v;
		for (float &color : vertex.color) color = 1;
		vertex.lightValue[0] = 15; vertex.lightValue[1] = 0;
		vertices.push_back(vertex);
	}
	if (m_vertices.isValid()) m_vertices.update(vertices.data(), u32(vertices.size() * sizeof(Vertex)));
	else m_vertices.init(vertices.data(), u32(vertices.size() * sizeof(Vertex)), true);
	states.texture = &m_texture;
	states.isCullFaceEnabled = false;
	target.draw(m_vertices, 0, u32(vertices.size()), states);
}
