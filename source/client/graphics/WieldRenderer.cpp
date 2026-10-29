#include <algorithm>
#include <cmath>
#include <vector>

#include <SDL_image.h>

#include "common/core/GameClock.hpp"
#include "common/core/Registry.hpp"
#include "common/resource/ResourceHandler.hpp"
#include "common/scene/RetroMobMesh.hpp"
#include "client/graphics/BgfxView.hpp"
#include "client/core/Config.hpp"
#include "client/graphics/RenderTarget.hpp"
#include "client/graphics/TextureAtlas.hpp"
#include "client/graphics/Vertex.hpp"
#include "client/graphics/WieldRenderer.hpp"
#include "client/gui/InventoryCube.hpp"
#include "client/world/ClientPlayer.hpp"

namespace {

constexpr float pi = 3.14159265358979323846f;
constexpr u64 swingDuration = 300;

void appendQuad(std::vector<Vertex> &vertices, const glm::vec3 (&points)[4], const FloatRect &uv) {
	constexpr int order[] = {0,1,2,0,2,3};
	for (int index : order) {
		Vertex vertex;
		for (int axis = 0; axis < 3; ++axis) vertex.coord3d[axis] = points[index][axis];
		vertex.texCoord[0] = uv.x + (index == 1 || index == 2 ? uv.sizeX : 0);
		vertex.texCoord[1] = uv.y + (index >= 2 ? uv.sizeY : 0);
		for (float &color : vertex.color) color = 1;
		vertices.push_back(vertex);
	}
}

}

WieldRenderer::WieldRenderer(ClientPlayer &player)
	: m_player(player), m_atlas(ResourceHandler::getInstance().get<TextureAtlas>("atlas-blocks")) {
	m_shader.loadFromFile("basic");
	m_skin.loadFromFile("resources/textures/player.png");
	m_armVertices.setupDefaultLayout();
	m_itemVertices.setupDefaultLayout();
	m_lastX = player.x();
	m_lastY = player.y();

	retro::MobMeshBuilder arm;
	arm.box(-2,0,-2,4,12,4,0,0,0,40,16);
	std::vector<Vertex> vertices;
	for (const auto &point : arm.vertices) {
		Vertex vertex;
		vertex.coord3d[0] = point.x;
		vertex.coord3d[1] = 1.125f - point.z;
		vertex.coord3d[2] = -point.y;
		vertex.texCoord[0] = point.u;
		vertex.texCoord[1] = point.v;
		for (float &color : vertex.color) color = 1;
		vertices.push_back(vertex);
	}
	m_armVertices.init(vertices.data(), u32(vertices.size() * sizeof(Vertex)));
}

WieldRenderer::~WieldRenderer() = default;

void WieldRenderer::pause() {
	m_attacking = false;
	m_using = false;
	m_swinging = false;
}

void WieldRenderer::onEvent(const SDL_Event &event) {
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) pause();
	if (event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEBUTTONUP) return;
	bool pressed = event.type == SDL_MOUSEBUTTONDOWN;
	if (event.button.button == SDL_BUTTON_LEFT) m_attacking = pressed;
	else if (event.button.button == SDL_BUTTON_RIGHT) m_using = pressed;
	else return;
	if (pressed && !m_swinging) {
		m_swingStart = GameClock::getInstance().getTicks();
		m_swinging = true;
	}
}

void WieldRenderer::setItem(const Item *item) {
	std::string id = item ? item->stringID() : "";
	if (m_itemID == id) return;
	m_itemID = id;
	m_cube.reset();
	m_itemVertexCount = 0;
	m_isTool = item && item->getGroupValue("group:om_tool_type");
	if (!item) return;
	if (item->isBlock() && Registry::getInstance().hasBlock(id)) {
		const Block &block = Registry::getInstance().getBlockFromStringID(id);
		const BlockState &state = block.getState(0);
		if (state.drawType() == BlockDrawType::XShape || !state.inventoryImage().empty()) {
			bool blockTexture = state.inventoryImage().empty();
			const std::string &filename = blockTexture ? state.tiles().getTextureForFace(0) : state.inventoryImage();
			buildItem(*item, filename, blockTexture);
		} else {
			m_cube = std::make_unique<InventoryCube>(0.5f, true);
			m_cube->updateVertexBuffer(block);
		}
	}
	else buildItem(*item, item->tiles().getTextureForFace(0), false);
}

void WieldRenderer::buildItem(const Item &item, const std::string &filename, bool blockTexture) {
	const FloatRect uv = m_atlas.getTexCoords(filename);
	std::vector<Vertex> vertices;
	constexpr float half = 0.28f, depth = 0.0175f;
	glm::vec3 front[4] = {{-half,half,depth},{half,half,depth},{half,-half,depth},{-half,-half,depth}};
	glm::vec3 back[4] = {{-half,half,-depth},{half,half,-depth},{half,-half,-depth},{-half,-half,-depth}};
	appendQuad(vertices, front, uv);
	appendQuad(vertices, back, uv);

	std::string path = "mods/" + item.modName() + (blockTexture ? "/textures/blocks/" : "/textures/items/") + filename;
	SDL_Surface *original = IMG_Load(path.c_str());
	SDL_Surface *surface = original ? SDL_ConvertSurfaceFormat(original, SDL_PIXELFORMAT_RGBA32, 0) : nullptr;
	if (original) SDL_FreeSurface(original);
	if (surface) {
		auto opaque = [&](int x, int y) {
			if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) return false;
			auto *pixels = static_cast<u8 *>(surface->pixels);
			return pixels[y * surface->pitch + x * 4 + 3] >= 76;
		};
		// outline edges give the sprite thickness without filling transparent pixels.
		for (int y = 0; y < surface->h; ++y) {
			for (int x = 0; x < surface->w; ++x) {
				if (!opaque(x,y)) continue;
				float left = -half + 2 * half * float(x) / float(surface->w);
				float right = -half + 2 * half * float(x + 1) / float(surface->w);
				float top = half - 2 * half * float(y) / float(surface->h);
				float bottom = half - 2 * half * float(y + 1) / float(surface->h);
				FloatRect pixel{uv.x + uv.sizeX * (float(x) + 0.5f) / float(surface->w),
					uv.y + uv.sizeY * (float(y) + 0.5f) / float(surface->h),0,0};
				if (!opaque(x - 1,y)) {
					glm::vec3 edge[4] = {{left,top,depth},{left,top,-depth},{left,bottom,-depth},{left,bottom,depth}};
					appendQuad(vertices,edge,pixel);
				}
				if (!opaque(x + 1,y)) {
					glm::vec3 edge[4] = {{right,top,depth},{right,top,-depth},{right,bottom,-depth},{right,bottom,depth}};
					appendQuad(vertices,edge,pixel);
				}
				if (!opaque(x,y - 1)) {
					glm::vec3 edge[4] = {{left,top,depth},{right,top,depth},{right,top,-depth},{left,top,-depth}};
					appendQuad(vertices,edge,pixel);
				}
				if (!opaque(x,y + 1)) {
					glm::vec3 edge[4] = {{left,bottom,depth},{right,bottom,depth},{right,bottom,-depth},{left,bottom,-depth}};
					appendQuad(vertices,edge,pixel);
				}
			}
		}
		SDL_FreeSurface(surface);
	}
	if (item.isBlock()) {
		const Color &color = Registry::getInstance().getBlockFromStringID(item.stringID()).getState(0).colorMultiplier();
		for (auto &vertex : vertices) {
			vertex.color[0] = color.r;
			vertex.color[1] = color.g;
			vertex.color[2] = color.b;
			vertex.color[3] = color.a;
		}
	}
	m_itemVertexCount = u32(vertices.size());
	m_itemVertices.init(vertices.data(), u32(vertices.size() * sizeof(Vertex)), true);
}

void WieldRenderer::update() {
	const Item *item = nullptr;
	if (m_player.heldItemSlot() >= 0) {
		const ItemStack &stack = m_player.heldItemStack();
		if (stack.amount() && stack.item().id()) item = &stack.item();
	}
	u64 now = GameClock::getInstance().getTicks();
	float elapsed = m_lastUpdate ? std::min(float(now - m_lastUpdate) / 1000.f, 0.05f) : 0;
	m_lastUpdate = now;
	std::string selected = item ? item->stringID() : "";
	if (selected != m_itemID) {
		m_equip = std::max(0.f, m_equip - elapsed * 8);
		if (m_equip == 0) setItem(item);
	} else m_equip = std::min(1.f, m_equip + elapsed * 8);
	if (m_swinging && now - m_swingStart >= swingDuration) {
		m_swinging = m_attacking || m_using;
		m_swingStart = now;
	}
	double distance = std::hypot(m_player.x() - m_lastX, m_player.y() - m_lastY);
	m_lastX = m_player.x();
	m_lastY = m_player.y();
	if (Config::isViewBobbingEnabled && m_player.onGround() && distance > 0.0001 && distance < 1) {
		m_walkPhase += float(distance) * 8;
		m_bob = std::sin(m_walkPhase) * 0.012f;
	}
	else m_bob *= 0.8f;
}

void WieldRenderer::draw(RenderTarget &target, RenderStates states) const {
	Vector2u size = target.getSize();
	if (!size.x || !size.y) return;
	float aspect = float(size.x) / float(size.y);
	m_camera.setAspectRatio(aspect);
	target.setView(m_camera);
	bgfx::setViewClear(BgfxView::Wield, BGFX_CLEAR_DEPTH, 0, 1.f);
	bgfx::setViewMode(BgfxView::Wield, bgfx::ViewMode::Sequential);
	states.view = BgfxView::Wield;
	states.shader = &m_shader;
	states.transform = Transform::Identity;
	states.isCullFaceEnabled = false;
	states.isDepthTestEnabled = true;

	u64 now = GameClock::getInstance().getTicks();
	float progress = m_swinging ? std::min(float(now - m_swingStart) / swingDuration, 1.f) : 0;
	float swing = std::sin(std::sqrt(progress) * pi);
	float strike = std::sin(progress * progress * pi);
	float x = std::min(aspect, 1.6f) * std::tan(36.f * pi / 180) * 1.15f * 0.58f;
	states.transform.translate(x - swing * 0.26f, -0.47f + m_bob - (1 - m_equip) * 0.8f
		+ std::sin(std::sqrt(progress) * 2 * pi) * 0.1f, -1.15f - strike * 0.15f);
	float widthScale = std::min(1.f, aspect / 1.3f);
	states.transform.scale(widthScale, widthScale, widthScale);
	if (m_cube) {
		states.transform.rotateY(-30.f - strike * 20.f);
		states.transform.rotateX(15.f - swing * 60.f);
		// block meshes use z up; the wield camera uses y up.
		states.transform.rotateX(-90.f);
		states.transform.translate(-0.25f,-0.25f,-0.25f);
		target.draw(*m_cube, states);
	}
	else if (m_itemVertexCount) {
		states.transform.rotateY(-30.f - strike * 20.f);
		states.transform.rotateZ((m_isTool ? 20.f : -15.f) - swing * 25.f);
		states.transform.rotateX(-swing * 65.f);
		if (m_isTool) {
			states.transform.scale(1.8f, 1.8f, 1.f);
			states.transform.translate(-0.10f, 0.08f, 0);
		}
		states.texture = &m_atlas.texture();
		target.draw(m_itemVertices, 0, m_itemVertexCount, states);
	}
	else {
		states.transform.translate(0.1f, -0.06f, 0);
		states.transform.rotateZ(-15.f - swing * 25.f);
		states.transform.rotateY(-20.f + strike * 30.f);
		states.transform.rotateX(-35.f - swing * 45.f);
		states.texture = &m_skin;
		target.draw(m_armVertices, 0, 36, states);
	}
}
