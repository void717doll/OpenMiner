#include "client/hud/HeartBar.hpp"
#include "client/graphics/RenderTarget.hpp"
#include "common/core/GameClock.hpp"

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

void HeartBar::update(unsigned health, u32 damageSerial) {
	health = std::min(health, hearts::count * 2);
	if (damageSerial != m_damageSerial) {
		m_previousHealth = std::min(m_health, hearts::count * 2);
		m_flashUntil = GameClock::getInstance().getTicks() + 600;
		m_damageSerial = damageSerial;
	}
	m_health = health;
	unsigned displayed = health;
	if (m_flashUntil) {
		u32 now = GameClock::getInstance().getTicks();
		if (now < m_flashUntil && (m_flashUntil - now) / 100 % 2) displayed = m_previousHealth;
		if (now >= m_flashUntil) m_flashUntil = 0;
	}
	if (displayed == m_displayedHealth) return;
	for (unsigned i = 0; i < hearts::count; ++i) {
		hearts::Fill state = hearts::fill(displayed, i);
		if (state != m_states[i] && state != hearts::Fill::Empty) {
			// clip the original artwork so odd health shows its left half.
			u16 width = state == hearts::Fill::Half ? 8 : 16;
			m_filled[i].setClipRect(0, 0, width, 16);
		}
		m_states[i] = state;
	}
	m_displayedHealth = displayed;
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
