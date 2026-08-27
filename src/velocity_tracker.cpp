#include "velocity_tracker.h"

namespace godot {

const double VelocityTracker::WINDOW_MS = 50.0;

void VelocityTracker::add_sample(float p_position, double p_time_ms) {
	m_samples.push_back({ p_position, p_time_ms });
	while (m_samples.size() > MAX_SAMPLES) {
		m_samples.remove_at(0);
	}
}

float VelocityTracker::get_velocity() const {
	if (m_samples.size() < 2) {
		return 0.0f;
	}
	const Sample &newest = m_samples[m_samples.size() - 1];
	// Oldest sample still inside the window; if every candidate is outside the
	// window we fall back to the very first sample.
	int oldest_idx = m_samples.size() - 2;
	for (int i = m_samples.size() - 2; i >= 0; i--) {
		if (newest.time_ms - m_samples[i].time_ms <= WINDOW_MS) {
			oldest_idx = i;
		} else {
			break;
		}
	}
	const Sample &oldest = m_samples[oldest_idx];
	const double dt_ms = newest.time_ms - oldest.time_ms;
	if (dt_ms <= 0.0) {
		return 0.0f;
	}
	return (float)((newest.position - oldest.position) / (dt_ms / 1000.0));
}

void VelocityTracker::clear() {
	m_samples.clear();
}

} // namespace godot
