#include <algorithm>
#include "common/resource/ResourceHandler.hpp"
#include "client/graphics/NinePatch.hpp"
#include "client/graphics/RenderTarget.hpp"

NinePatch::NinePatch(const std::string &texture, u16 border)
	: NinePatch(ResourceHandler::getInstance().get<Texture>(texture), border) {}

NinePatch::NinePatch(const Texture &source, u16 border) {
	m_sourceWidth = u16(source.getSize().x);
	m_sourceHeight = u16(source.getSize().y);
	m_border = std::min<u16>(border, std::min(m_sourceWidth, m_sourceHeight) / 2);
	for (auto &part : m_parts) part.load(source);
	setSize(m_sourceWidth, m_sourceHeight);
}

void NinePatch::setSize(u16 width, u16 height) {
	u16 border = std::min<u16>(m_border, std::min(width, height) / 2);
	u16 sourceX[] = {0, m_border, u16(m_sourceWidth - m_border)};
	u16 sourceY[] = {0, m_border, u16(m_sourceHeight - m_border)};
	u16 sourceWidth[] = {m_border, u16(m_sourceWidth - 2 * m_border), m_border};
	u16 sourceHeight[] = {m_border, u16(m_sourceHeight - 2 * m_border), m_border};
	u16 x[] = {0, border, u16(width - border)}, y[] = {0, border, u16(height - border)};
	u16 widths[] = {border, u16(width - 2 * border), border};
	u16 heights[] = {border, u16(height - 2 * border), border};
	for (unsigned row = 0; row < 3; ++row) {
		for (unsigned column = 0; column < 3; ++column) {
			auto &part = m_parts[row * 3 + column];
			part.setClipRect(sourceX[column], sourceY[row], sourceWidth[column], sourceHeight[row]);
			part.setPosRect(x[column], y[row], widths[column], heights[row]);
		}
	}
}

void NinePatch::setColor(const Color &color) {
	for (auto &part : m_parts) part.setColor(color);
}

void NinePatch::draw(RenderTarget &target, RenderStates states) const {
	states.transform *= getTransform();
	for (const auto &part : m_parts) target.draw(part, states);
}
