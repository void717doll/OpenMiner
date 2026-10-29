#include <algorithm>

#include "client/hud/ArmorBar.hpp"
#include "client/graphics/RenderTarget.hpp"

ArmorBar::ArmorBar() {
	for (unsigned i = 0; i < m_empty.size(); ++i) {
		m_empty[i].load("texture-armor_empty");
		m_filled[i].load("texture-armor_full");
		m_empty[i].setScale(9.f / 16.f, 9.f / 16.f);
		m_filled[i].setScale(9.f / 16.f, 9.f / 16.f);
		m_empty[i].setPosition(float(i) * 8.f, 0);
		m_filled[i].setPosition(float(i) * 8.f, 0);
	}
}

void ArmorBar::update(unsigned protection) {
	protection = std::min(protection, 20u);
	if (protection == m_protection) return;
	for (unsigned i = 0; i < m_filled.size(); ++i)
		m_filled[i].setClipRect(0, 0, protection == i * 2 + 1 ? 8 : 16, 16);
	m_protection = protection;
}

void ArmorBar::draw(RenderTarget &target, RenderStates states) const {
	if (!m_protection) return;
	states.transform *= getTransform();
	for (unsigned i = 0; i < m_empty.size(); ++i) {
		if (m_protection < i * 2 + 2)
			target.draw(m_empty[i], states);
		if (m_protection > i * 2)
			target.draw(m_filled[i], states);
	}
}
