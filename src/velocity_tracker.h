#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Lightweight drag-velocity estimator. It is fed (position, time) samples
// during a drag and estimates the release velocity on demand. Mirrors the
// spirit of Android's VelocityTracker: the newest sample is paired with the
// oldest sample still inside a short tracking window, so a brief pause right
// before release does not drag the estimate to zero.
class VelocityTracker {
public:
	struct Sample {
		float position;
		double time_ms;
	};

	void add_sample(float p_position, double p_time_ms);
	// Estimated velocity in px/s, or 0 when there are fewer than two samples
	// with a nonzero time spread.
	float get_velocity() const;
	void clear();

private:
	static const int MAX_SAMPLES = 16;
	static const double WINDOW_MS;
	Vector<Sample> m_samples;
};

} // namespace godot
