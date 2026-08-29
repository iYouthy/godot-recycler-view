#include "snap_helper.h"

#include "fling_scroller.h"
#include "recycler_view.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <limits>

namespace godot {

// --- SnapHelper base ---

void SnapHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("attach_to_recycler_view", "recycler_view"), &SnapHelper::attach_to_recycler_view);
	ClassDB::bind_method(D_METHOD("detach"), &SnapHelper::detach);
	ClassDB::bind_method(D_METHOD("on_fling", "velocity"), &SnapHelper::on_fling);
}

SnapHelper::SnapHelper() {}

SnapHelper::~SnapHelper() {}

void SnapHelper::attach_to_recycler_view(RecyclerView *p_recycler_view) {
	if (m_recycler_view != nullptr && m_recycler_view != p_recycler_view) {
		m_recycler_view->set_snap_helper(Ref<SnapHelper>());
	}
	m_recycler_view = p_recycler_view;
	if (m_recycler_view != nullptr) {
		m_recycler_view->set_snap_helper(Ref<SnapHelper>(this));
		if (m_listener.is_null()) {
			Ref<SnapScrollListener> listener;
			listener.instantiate();
			listener->owner = this;
			m_listener = listener;
		}
		m_recycler_view->add_on_scroll_listener(m_listener);
		// Snap the initially-visible content into place (Android does this on
		// attach so a half-scrolled list centers immediately).
		snap_to_target_existing_view();
	}
}

void SnapHelper::detach() {
	if (m_recycler_view != nullptr) {
		if (m_listener.is_valid()) {
			m_recycler_view->remove_on_scroll_listener(m_listener);
		}
		m_recycler_view->set_snap_helper(Ref<SnapHelper>());
	}
	m_recycler_view = nullptr;
}

void SnapHelper::on_recycler_view_destroyed() {
	m_recycler_view = nullptr;
	m_scrolled = false;
}

bool SnapHelper::is_horizontal(RecyclerView *p_recycler_view) {
	return p_recycler_view->get_layout().is_valid() && p_recycler_view->get_layout()->can_scroll_horizontally();
}

float SnapHelper::axis_of(RecyclerView *p_recycler_view, const Vector2 &p_v) {
	return is_horizontal(p_recycler_view) ? p_v.x : p_v.y;
}

float SnapHelper::container_center(RecyclerView *p_recycler_view) {
	const Vector2 viewport = p_recycler_view->get_viewport_size();
	return axis_of(p_recycler_view, viewport) / 2.0f;
}

int SnapHelper::current_scroll_offset(RecyclerView *p_recycler_view) {
	return is_horizontal(p_recycler_view)
			? p_recycler_view->get_scroll_offset_horizontal()
			: p_recycler_view->get_scroll_offset();
}

float SnapHelper::distance_to_center(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) const {
	const Vector2 pos = p_recycler_view->get_layout_position(p_holder);
	const Vector2 size = p_holder->get_control() != nullptr ? p_holder->get_control()->get_size() : Vector2();
	const float child_center = axis_of(p_recycler_view, pos) + axis_of(p_recycler_view, size) / 2.0f;
	return child_center - container_center(p_recycler_view);
}

Ref<ViewHolder> SnapHelper::find_snap_view(RecyclerView *p_recycler_view) {
	Ref<ViewHolder> closest;
	float best = std::numeric_limits<float>::max();
	const int count = p_recycler_view->get_child_holder_count();
	for (int i = 0; i < count; i++) {
		Ref<ViewHolder> holder = p_recycler_view->get_child_holder_at(i);
		const float dist = Math::abs(distance_to_center(p_recycler_view, holder));
		if (dist < best) {
			best = dist;
			closest = holder;
		}
	}
	return closest;
}

int SnapHelper::find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity) {
	// Base default: no fling-based jump (Linear/Pager override). The scroll-end
	// snap still centers after the fling stops.
	return NO_POSITION;
}

int SnapHelper::calculate_distance_to_final_snap(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) {
	return (int)distance_to_center(p_recycler_view, p_holder);
}

int SnapHelper::calculate_time_for_scrolling(int p_dx) const {
	// Android's LinearSmoothScroller: MILLISECONDS_PER_INCH (100) / densityDpi.
	// The port has no pixel density; 160dpi is the mdpi baseline (density 1.0).
	static constexpr float SPEED_PER_PIXEL_MS = 100.0f / 160.0f;
	return (int)Math::ceil(Math::abs((float)p_dx) * SPEED_PER_PIXEL_MS);
}

int SnapHelper::settle_duration_ms(int p_distance) const {
	const int scroll_ms = calculate_time_for_scrolling(p_distance);
	// Android's LinearSmoothScroller decelerates into the target in
	// calculateTimeForDeceleration = timeForScrolling / 0.3356.
	return (int)Math::ceil(scroll_ms / 0.3356);
}

int SnapHelper::fling_settle_duration_ms(int p_distance) const {
	// The long-haul scroll uses the linear time (Android's timeForScrolling);
	// only the final centering decelerates (settle_duration_ms). Pager's
	// calculate_time_for_scrolling override already caps this.
	return calculate_time_for_scrolling(p_distance);
}

int SnapHelper::target_offset_for(RecyclerView *p_recycler_view, int p_position) const {
	const int start = p_recycler_view->get_layout()->get_position_offset(p_position);
	const float item_size = (float)p_recycler_view->get_item_height(p_position);
	return start + (int)(item_size / 2.0f - container_center(p_recycler_view));
}

bool SnapHelper::on_fling(float p_velocity) {
	if (m_recycler_view == nullptr || m_recycler_view->get_layout().is_null()) {
		return false;
	}
	if (Math::abs(p_velocity) < MIN_FLING_VELOCITY) {
		return false;
	}
	const int target = find_target_snap_position(m_recycler_view, p_velocity);
	if (target == NO_POSITION) {
		// No page estimate (e.g. a weak fling rounds to the current page): let
		// the RV fling; the scroll-end snap will center whatever it settles on.
		return false;
	}
	const int target_offset = target_offset_for(m_recycler_view, target);
	const int duration_ms = fling_settle_duration_ms(Math::abs(target_offset - current_scroll_offset(m_recycler_view)));
	m_recycler_view->smooth_scroll_to(target_offset, duration_ms / 1000.0);
	return true;
}

void SnapHelper::snap_to_target_existing_view() {
	if (m_recycler_view == nullptr || m_recycler_view->get_layout().is_null()) {
		return;
	}
	const Ref<ViewHolder> snap_view = find_snap_view(m_recycler_view);
	if (snap_view.is_null()) {
		return;
	}
	const int distance = calculate_distance_to_final_snap(m_recycler_view, snap_view);
	if (distance == 0) {
		return;
	}
	const int current = current_scroll_offset(m_recycler_view);
	const int duration_ms = settle_duration_ms(Math::abs(distance));
	m_recycler_view->smooth_scroll_to(current + distance, duration_ms / 1000.0);
}

void SnapHelper::SnapScrollListener::on_scroll_state_changed(int p_state) {
	if (owner == nullptr) {
		return;
	}
	if (p_state == RecyclerView::SCROLL_STATE_IDLE && owner->m_scrolled) {
		owner->m_scrolled = false;
		owner->snap_to_target_existing_view();
	}
}

void SnapHelper::SnapScrollListener::on_scrolled(int p_dx, int p_dy) {
	if (owner != nullptr && (p_dx != 0 || p_dy != 0)) {
		owner->m_scrolled = true;
	}
}

// --- LinearSnapHelper ---

void LinearSnapHelper::_bind_methods() {}

Ref<ViewHolder> LinearSnapHelper::find_snap_view(RecyclerView *p_recycler_view) {
	return SnapHelper::find_snap_view(p_recycler_view);
}

int LinearSnapHelper::calculate_distance_to_final_snap(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) {
	return (int)distance_to_center(p_recycler_view, p_holder);
}

int LinearSnapHelper::find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity) {
	const int item_count = p_recycler_view->get_layout()->get_item_count();
	if (item_count == 0) {
		return NO_POSITION;
	}
	const Ref<ViewHolder> snap_view = find_snap_view(p_recycler_view);
	if (snap_view.is_null()) {
		return NO_POSITION;
	}
	const int current = snap_view->get_position();
	const int delta_jump = estimate_next_position_diff_for_fling(p_recycler_view, p_velocity);
	if (delta_jump == 0) {
		return NO_POSITION;
	}
	return CLAMP(current + delta_jump, 0, item_count - 1);
}

int LinearSnapHelper::estimate_next_position_diff_for_fling(RecyclerView *p_recycler_view, float p_velocity) const {
	const int distance_per_child = compute_distance_per_child(p_recycler_view);
	if (distance_per_child == INVALID_DISTANCE) {
		return 0;
	}
	// Signed inertial distance the RV's own fling would travel for this velocity
	// (same spline physics), divided by the average item size.
	const int distance = FlingScroller::predict_end_distance(p_velocity);
	return (int)Math::round((float)distance / distance_per_child);
}

int LinearSnapHelper::compute_distance_per_child(RecyclerView *p_recycler_view) const {
	const int count = p_recycler_view->get_child_holder_count();
	if (count < 2) {
		return INVALID_DISTANCE;
	}
	int min_pos = std::numeric_limits<int>::max();
	int max_pos = std::numeric_limits<int>::min();
	float min_start = std::numeric_limits<float>::max();
	float max_end = -std::numeric_limits<float>::max();
	for (int i = 0; i < count; i++) {
		const Ref<ViewHolder> holder = p_recycler_view->get_child_holder_at(i);
		const int pos = holder->get_position();
		const float start = axis_of(p_recycler_view, p_recycler_view->get_layout_position(holder));
		const float size = axis_of(p_recycler_view, holder->get_control() != nullptr ? holder->get_control()->get_size() : Vector2());
		if (pos < min_pos) {
			min_pos = pos;
			min_start = start;
		}
		if (pos > max_pos) {
			max_pos = pos;
			max_end = start + size;
		}
	}
	if (max_pos == min_pos) {
		return INVALID_DISTANCE;
	}
	return (int)((max_end - min_start) / (max_pos - min_pos + 1));
}

int LinearSnapHelper::fling_settle_duration_ms(int p_distance) const {
	// Cap the linear fling time: a strong fling across many pages still parks
	// within ~a second instead of dragging on for the full linear duration.
	return MIN(MAX_FLING_SETTLE_DURATION_MS, calculate_time_for_scrolling(p_distance));
}

// --- PagerSnapHelper ---

void PagerSnapHelper::_bind_methods() {}

int PagerSnapHelper::find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity) {
	const int item_count = p_recycler_view->get_layout()->get_item_count();
	if (item_count == 0) {
		return NO_POSITION;
	}
	// Positive effective velocity scrolls forward (later positions); the RV's
	// fling uses the same convention.
	const bool forward = p_velocity > 0.0f;
	const int count = p_recycler_view->get_child_holder_count();
	Ref<ViewHolder> before;
	Ref<ViewHolder> after;
	float best_before = -std::numeric_limits<float>::max();
	float best_after = std::numeric_limits<float>::max();
	for (int i = 0; i < count; i++) {
		const Ref<ViewHolder> holder = p_recycler_view->get_child_holder_at(i);
		const float dist = distance_to_center(p_recycler_view, holder);
		if (dist <= 0.0f && dist > best_before) {
			best_before = dist;
			before = holder;
		}
		if (dist >= 0.0f && dist < best_after) {
			best_after = dist;
			after = holder;
		}
	}
	const Ref<ViewHolder> target = forward ? after : before;
	if (target.is_valid()) {
		return target->get_position();
	}
	// No child on the requested side (list edge): step one position past the
	// visible range. Clamped, so a fling at the last page stays on it.
	if (count > 0) {
		const int edge_pos = forward
				? p_recycler_view->get_child_holder_at(count - 1)->get_position()
				: p_recycler_view->get_child_holder_at(0)->get_position();
		return CLAMP(edge_pos + (forward ? 1 : -1), 0, item_count - 1);
	}
	return NO_POSITION;
}

int PagerSnapHelper::calculate_time_for_scrolling(int p_dx) const {
	// Android caps the fling scroll duration at MAX_SCROLL_ON_FLING_DURATION
	// (100ms) so a page turn is snappy, never a long glide.
	return MIN(MAX_SCROLL_ON_FLING_DURATION, SnapHelper::calculate_time_for_scrolling(p_dx));
}

} // namespace godot
