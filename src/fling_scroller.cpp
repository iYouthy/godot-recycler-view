#include "fling_scroller.h"

#include <cmath>

namespace godot {
namespace {

constexpr int NB_SAMPLES = 100;
// Spline control points (cubic Bezier), verbatim from SplineOverScroller.
constexpr float START_TENSION = 0.5f;
constexpr float END_TENSION = 1.0f;
constexpr float INFLEXION = 0.35f;
constexpr float P1 = START_TENSION * INFLEXION;
constexpr float P2 = 1.0f - END_TENSION * (1.0f - INFLEXION);
constexpr float FLING_FRICTION = 0.015f;
// g (m/s^2) * inch/meter * density-independent px/inch (density=1) * tuning.
constexpr float PHYSICAL_COEFF = 9.80665f * 39.37f * 160.0f * 0.84f;
const double DECELERATION_RATE = std::log(0.78) / std::log(0.9);

} // namespace

void FlingScroller::fling(int p_start, float p_velocity, int p_min, int p_max) {
	m_finished = false;
	m_was_clamped = false;
	m_curr_velocity = p_velocity;
	m_time_ms = 0.0;
	if (p_start < p_min || p_start > p_max) {
		// No overscroll state in this port; clamp the start into range.
		if (p_start < p_min) {
			p_start = p_min;
		} else {
			p_start = p_max;
		}
	}
	m_current_position = p_start;
	m_start = p_start;

	double total_distance = 0.0;
	m_duration = m_spline_duration = 0;
	if (p_velocity != 0.0f) {
		m_spline_duration = get_spline_fling_duration(p_velocity);
		m_duration = m_spline_duration;
		total_distance = get_spline_fling_distance(p_velocity);
	}

	m_spline_distance = (int)(total_distance * (p_velocity > 0.0f ? 1.0 : -1.0));
	m_final = p_start + m_spline_distance;

	if (m_final < p_min) {
		m_was_clamped = true;
		adjust_duration(m_start, m_final, p_min);
		m_final = p_min;
	}
	if (m_final > p_max) {
		m_was_clamped = true;
		adjust_duration(m_start, m_final, p_max);
		m_final = p_max;
	}
}

bool FlingScroller::update(double p_delta_ms) {
	if (m_finished) {
		return true;
	}
	m_time_ms += p_delta_ms;
	const double current_time = m_time_ms;
	if (current_time == 0.0) {
		// Skip the work but report that we are still going for a nonzero
		// duration, mirroring the original.
		return m_duration > 0;
	}
	if (current_time > (double)m_duration) {
		// Settled: land exactly on the (clamped) target, like OverScroller's
		// computeScrollOffset() forcing mCurr onto mFinal.
		m_finished = true;
		m_current_position = m_final;
		m_curr_velocity = 0.0f;
		return false;
	}

	const float t = (float)(current_time / (double)m_spline_duration);
	const float *table = spline_position();
	const int index = (int)((float)NB_SAMPLES * t);
	float distance_coef = 1.0f;
	float velocity_coef = 0.0f;
	if (index < NB_SAMPLES) {
		const float t_inf = (float)index / (float)NB_SAMPLES;
		const float t_sup = (float)(index + 1) / (float)NB_SAMPLES;
		const float d_inf = table[index];
		const float d_sup = table[index + 1];
		velocity_coef = (d_sup - d_inf) / (t_sup - t_inf);
		distance_coef = d_inf + (t - t_inf) * velocity_coef;
	}
	const float distance = distance_coef * (float)m_spline_distance;
	m_curr_velocity = velocity_coef * (float)m_spline_distance / (float)m_spline_duration * 1000.0f;
	m_current_position = m_start + (int)std::round(distance);
	return true;
}

void FlingScroller::stop() {
	m_finished = true;
}

int FlingScroller::predict_end_distance(float p_velocity) {
	FlingScroller tmp;
	const double total = tmp.get_spline_fling_distance(p_velocity);
	return (int)(total * (p_velocity > 0.0f ? 1.0 : -1.0));
}

double FlingScroller::get_spline_deceleration(float p_velocity) const {
	return std::log(INFLEXION * std::fabs(p_velocity) / (FLING_FRICTION * PHYSICAL_COEFF));
}

double FlingScroller::get_spline_fling_distance(float p_velocity) const {
	const double l = get_spline_deceleration(p_velocity);
	const double decel_minus_one = DECELERATION_RATE - 1.0;
	return (double)FLING_FRICTION * (double)PHYSICAL_COEFF * std::exp(DECELERATION_RATE / decel_minus_one * l);
}

int FlingScroller::get_spline_fling_duration(float p_velocity) const {
	const double l = get_spline_deceleration(p_velocity);
	const double decel_minus_one = DECELERATION_RATE - 1.0;
	return (int)(1000.0 * std::exp(l / decel_minus_one));
}

void FlingScroller::adjust_duration(int p_start, int p_old_final, int p_new_final) {
	const double x = std::fabs((double)p_old_final - (double)p_start) / (double)std::fabs((double)m_spline_distance);
	const int x_duration = (int)(x * (double)m_spline_duration);
	m_duration = (int)((double)x_duration * (double)std::fabs((double)p_new_final - (double)p_start) / std::fabs((double)p_old_final - (double)p_start));
	m_spline_distance = p_new_final - p_start;
}

const float *FlingScroller::spline_position() {
	static float s_table[NB_SAMPLES + 1];
	static bool s_ready = false;
	if (s_ready) {
		return s_table;
	}
	// Port of the SplineOverScroller static initializer: bisect each bezier
	// sample so the sample x-coordinate is alpha, then evaluate the position.
	float x_min = 0.0f;
	for (int i = 0; i < NB_SAMPLES; i++) {
		const float alpha = (float)i / (float)NB_SAMPLES;
		float x_max = 1.0f;
		float x = 0.0f;
		float coef = 0.0f;
		while (true) {
			x = x_min + (x_max - x_min) / 2.0f;
			coef = 3.0f * x * (1.0f - x);
			const float tx = coef * ((1.0f - x) * P1 + x * P2) + x * x * x;
			if (std::fabs(tx - alpha) < 1e-5f) {
				break;
			}
			if (tx > alpha) {
				x_max = x;
			} else {
				x_min = x;
			}
		}
		s_table[i] = coef * ((1.0f - x) * START_TENSION + x) + x * x * x;
	}
	s_table[NB_SAMPLES] = 1.0f;
	s_ready = true;
	return s_table;
}

} // namespace godot
