#ifndef SPYGLASSSTATE_HPP_
#define SPYGLASSSTATE_HPP_

class SpyglassState {
public:
	void begin(int slot, unsigned dimension) { m_active = true; m_slot = slot; m_dimension = dimension; }
	void reset() { m_active = false; }
	void update(bool playing, bool holdingSpyglass, int slot, unsigned dimension) {
		if (!playing || !holdingSpyglass || slot != m_slot || dimension != m_dimension) reset();
	}
	bool active() const { return m_active; }
	float fieldOfView(float configured) const { return m_active ? 8.f : configured; }
private:
	bool m_active = false;
	int m_slot = -1;
	unsigned m_dimension = 0;
};

#endif // SPYGLASSSTATE_HPP_
