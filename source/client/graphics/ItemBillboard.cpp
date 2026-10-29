#include <glm/gtc/matrix_inverse.hpp>

#include "common/core/Registry.hpp"
#include "common/resource/ResourceHandler.hpp"
#include "client/graphics/ItemBillboard.hpp"
#include "client/graphics/RenderTarget.hpp"
#include "client/graphics/Vertex.hpp"

ItemBillboard::ItemBillboard(const std::string &item, float size)
	: m_atlas(ResourceHandler::getInstance().get<TextureAtlas>("atlas-blocks")), m_size(size) {
	const Item &definition = Registry::getInstance().getItemFromStringID(item);
	m_uv = m_atlas.getTexCoords(definition.tiles().getTextureForFace(0));
	m_vertices.setupDefaultLayout();
}

void ItemBillboard::draw(RenderTarget &target, RenderStates states) const {
	if (!target.getView()) return;
	const auto &view = target.getView()->getViewTransform().getMatrix();
	glm::mat3 local = glm::inverse(glm::mat3(states.transform.getMatrix()));
	glm::vec3 right = local * glm::vec3(view[0][0], view[1][0], view[2][0]);
	glm::vec3 up = local * glm::vec3(view[0][1], view[1][1], view[2][1]);
	constexpr int indices[6] = {0, 1, 2, 0, 2, 3};
	constexpr float corners[4][2] = {{-0.5f,0.5f},{-0.5f,-0.5f},{0.5f,-0.5f},{0.5f,0.5f}};
	Vertex vertices[6];
	for (int i = 0; i < 6; ++i) {
		int corner = indices[i];
		glm::vec3 point = (right * corners[corner][0] + up * corners[corner][1]) * m_size;
		vertices[i].coord3d[0] = point.x;
		vertices[i].coord3d[1] = point.y;
		vertices[i].coord3d[2] = point.z;
		vertices[i].texCoord[0] = m_uv.x + (corner >= 2 ? m_uv.sizeX : 0);
		vertices[i].texCoord[1] = m_uv.y + (corner == 1 || corner == 2 ? m_uv.sizeY : 0);
		for (float &color : vertices[i].color) color = 1;
		vertices[i].lightValue[0] = 15;
		vertices[i].lightValue[1] = 0;
	}
	if (m_vertices.isValid()) m_vertices.update(vertices, sizeof(vertices));
	else m_vertices.init(vertices, sizeof(vertices), true);
	states.texture = &m_atlas.texture();
	states.isCullFaceEnabled = false;
	states.isDepthTestEnabled = true;
	target.draw(m_vertices, 0, 6, states);
}
