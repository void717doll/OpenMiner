#include "client/hud/HeartBar.hpp"
#include "client/graphics/RenderTarget.hpp"

HeartBar::HeartBar() {
	for (unsigned i = 0; i < hearts::count; ++i) {
		m_empty[i].load("texture-heart_empty");
		m_filled[i].load("texture-heart_full");
		m_empty[i].setScale(hearts::size / 16.f, hearts::size / 16.f);
		m_filled[i].setScale(hearts::size / 16.f, hearts::size / 16.f);
		m_empty[i].setPosition(float(i) * hearts::spacing, 0);
		m_filled[i].setPosition(float(i) * hearts::spacing, 0);
	}
	update(20);
}

void HeartBar::update(unsigned health) {
	health = std::min(health, hearts::count * 2);
	if (health == m_health) return;
	for (unsigned i = 0; i < hearts::count; ++i) {
		hearts::Fill state = hearts::fill(health, i);
		if (state != m_states[i] && state != hearts::Fill::Empty) {
			// clip the original artwork so odd health shows its left half.
			u16 width = state == hearts::Fill::Half ? 8 : 16;
			m_filled[i].setClipRect(0, 0, width, 16);
		}
		m_states[i] = state;
	}
	m_health = health;
}

void HeartBar::draw(RenderTarget &target, RenderStates states) const {
	states.transform *= getTransform();
	for (unsigned i = 0; i < hearts::count; ++i) {
		if (m_states[i] != hearts::Fill::Full)
			target.draw(m_empty[i], states);
		if (m_states[i] != hearts::Fill::Empty)
			target.draw(m_filled[i], states);
	}
}
