#pragma once

namespace godot {

// One-axis inertial fling: a faithful port of AOSP SplineOverScroller's SPLINE
// state (android/widget/OverScroller.java). Pure math, doctest-testable.
// Unlike Android this is driven externally through update(delta_ms) instead of
// the wall clock, so tests stay deterministic. There is no overscroll state:
// the fling clamps to [min, max] and settles exactly on the boundary.
class FlingScroller {
public:
	void fling(int p_start, float p_velocity, int p_min, int p_max);

	// Advances the fling by p_delta_ms. Returns false once the fling has
	// settled; on that frame get_current_position() lands exactly on the
	// clamped target.
	bool update(double p_delta_ms);

	void stop();

	bool is_finished() const { return m_finished; }
	int get_current_position() const { return m_current_position; }
	float get_current_velocity() const { return m_curr_velocity; }
	int get_final_position() const { return m_final; }
	int get_start_position() const { return m_start; }
	int get_duration() const { return m_duration; }
	// True when fling() had to clamp the target onto [min, max] because the
	// natural spline distance would have crossed the boundary. Nested scroll
	// relay uses this to detect momentum that spills into an ancestor.
	bool was_clamped() const { return m_was_clamped; }

	// Lazily built 101-entry distance/time table (index 0..100). Public for the
	// doctests to validate its shape (monotonic, anchored at 0 and 1).
	static const float *spline_position();

private:
	double get_spline_deceleration(float p_velocity) const;
	double get_spline_fling_distance(float p_velocity) const;
	int get_spline_fling_duration(float p_velocity) const;
	void adjust_duration(int p_start, int p_old_final, int p_new_final);

	bool m_finished = true;
	int m_current_position = 0;
	int m_start = 0;
	int m_final = 0;
	// m_duration is the effective duration; m_spline_duration is the one used
	// for the time->position curve. They differ when the final position was
	// clamped to a boundary (adjust_duration shortens the effective duration).
	int m_duration = 0;
	int m_spline_duration = 0;
	int m_spline_distance = 0;
	float m_curr_velocity = 0.0f;
	double m_time_ms = 0.0;
	bool m_was_clamped = false;
};

} // namespace godot
