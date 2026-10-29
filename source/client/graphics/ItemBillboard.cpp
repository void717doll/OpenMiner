#include "common/core/Registry.hpp"
#include "common/resource/ResourceHandler.hpp"
#include "client/graphics/ItemBillboard.hpp"
#include "client/graphics/ItemMeshBuilder.hpp"
#include "client/graphics/RenderTarget.hpp"

ItemBillboard::ItemBillboard(const std::string &item, float size)
	: m_atlas(ResourceHandler::getInstance().get<TextureAtlas>("atlas-blocks")) {
	const Item &definition = Registry::getInstance().getItemFromStringID(item);
	std::string filename = definition.tiles().getTextureForFace(0);
	bool blockTexture = definition.isBlock();
	if (blockTexture) {
		const BlockState &state = Registry::getInstance().getBlockFromStringID(item).getState(0);
		blockTexture = state.inventoryImage().empty();
		filename = blockTexture ? state.tiles().getTextureForFace(0) : state.inventoryImage();
	}
	m_mesh = buildItemMesh(definition, m_atlas, filename, blockTexture, size);
	for (auto &vertex : m_mesh) {
		float height = vertex.coord3d[1];
		vertex.coord3d[1] = -vertex.coord3d[2];
		vertex.coord3d[2] = height;
		vertex.lightValue[0] = m_sunlight;
		vertex.lightValue[1] = m_torchlight;
	}
	m_vertices.setupDefaultLayout();
	m_vertices.init(m_mesh.data(), u32(m_mesh.size() * sizeof(Vertex)), true);
}

void ItemBillboard::setLight(u8 sunlight, u8 torchlight) {
	if (m_sunlight == sunlight && m_torchlight == torchlight) return;
	m_sunlight = sunlight;
	m_torchlight = torchlight;
	for (auto &vertex : m_mesh) {
		vertex.lightValue[0] = sunlight;
		vertex.lightValue[1] = torchlight;
	}
	m_vertices.update(m_mesh.data(), u32(m_mesh.size() * sizeof(Vertex)));
}

void ItemBillboard::draw(RenderTarget &target, RenderStates states) const {
	states.texture = &m_atlas.texture();
	states.isCullFaceEnabled = false;
	states.isDepthTestEnabled = true;
	target.draw(m_vertices, 0, u32(m_mesh.size()), states);
}
