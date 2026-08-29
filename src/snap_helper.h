#pragma once

#include "scroll_listener.h"
#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class RecyclerView;

// Port of RecyclerView.SnapHelper: after a fling or scroll, settles the list so
// a target item is aligned. LinearSnapHelper centers the item closest to the
// viewport center; PagerSnapHelper snaps one whole page per fling. Attach with
// attach_to_recycler_view(). Two trigger paths mirror Android: it takes over a
// fling from the RV (on_fling returns true), and a scroll listener snaps after
// any scroll stops (SCROLL_STATE_IDLE). Settles run through the RV's
// smooth_scroll_to decelerate animation.
class SnapHelper : public RefCounted {
	GDCLASS(SnapHelper, RefCounted)

protected:
	static void _bind_methods();

public:
	SnapHelper();
	~SnapHelper() override;

	void attach_to_recycler_view(RecyclerView *p_recycler_view);
	void detach();
	// Called by the RecyclerView destructor so a dangling rv pointer is never
	// dereferenced after the RV is freed.
	void on_recycler_view_destroyed();

	// Called by the RV when it would start a fling (effective velocity along the
	// scroll axis). Returns true when the helper took over: it smooth-scrolls to
	// a snapped target and the RV must not run its own fling.
	bool on_fling(float p_velocity);

	// Centers on the currently best-matching visible item. Called by the scroll
	// listener when a scroll settles, and at attach time.
	void snap_to_target_existing_view();

	// Subclass hooks (Android's three abstract SnapHelper methods).
	virtual Ref<ViewHolder> find_snap_view(RecyclerView *p_recycler_view);
	virtual int find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity);
	virtual int calculate_distance_to_final_snap(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder);

protected:
	// Duration (ms) of the linear scroll for the given distance; Pager caps it
	// so a page turn is snappy. Subclass override point.
	virtual int calculate_time_for_scrolling(int p_dx) const;
	// Duration of a fling-driven settle for the given distance. Android uses the
	// linear scroll time for the long haul (only the final centering decelerates,
	// see settle_duration_ms); subclasses cap it so a strong fling does not drag
	// on for seconds.
	virtual int fling_settle_duration_ms(int p_distance) const;

	static constexpr float MIN_FLING_VELOCITY = 50.0f;

	// Scroll axis helpers (layout's primary axis).
	static bool is_horizontal(RecyclerView *p_recycler_view);
	static float axis_of(RecyclerView *p_recycler_view, const Vector2 &p_v);
	static float container_center(RecyclerView *p_recycler_view);
	static int current_scroll_offset(RecyclerView *p_recycler_view);
	float distance_to_center(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) const;
	// The scroll offset that centers the item at the given position.
	int target_offset_for(RecyclerView *p_recycler_view, int p_position) const;
	int settle_duration_ms(int p_distance) const;

	RecyclerView *m_recycler_view = nullptr;
	Ref<ScrollListener> m_listener;

private:
	class SnapScrollListener : public ScrollListener {
	public:
		SnapHelper *owner = nullptr;
		void on_scroll_state_changed(int p_state) override;
		void on_scrolled(int p_dx, int p_dy) override;
	};

	bool m_scrolled = false;
};

// LinearSnapHelper: settles the item closest to the viewport center. A fling
// estimates how many items it would cross (using the same spline physics as the
// RV's own fling) and jumps to that position.
class LinearSnapHelper : public SnapHelper {
	GDCLASS(LinearSnapHelper, SnapHelper)

protected:
	static void _bind_methods();

public:
	Ref<ViewHolder> find_snap_view(RecyclerView *p_recycler_view) override;
	int find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity) override;
	int calculate_distance_to_final_snap(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) override;

private:
	static constexpr int INVALID_DISTANCE = 1;
	// A strong fling must not drag on for seconds: cap the settle so the list
	// parks on the estimated page within roughly a second.
	static constexpr int MAX_FLING_SETTLE_DURATION_MS = 1200;
	int estimate_next_position_diff_for_fling(RecyclerView *p_recycler_view, float p_velocity) const;
	int compute_distance_per_child(RecyclerView *p_recycler_view) const;

public:
	int fling_settle_duration_ms(int p_distance) const override;
};

// PagerSnapHelper: snaps one whole page per fling. The velocity only picks the
// direction (forward/backward); the target is the child nearest the viewport
// center on that side, so a page turn never skips.
class PagerSnapHelper : public SnapHelper {
	GDCLASS(PagerSnapHelper, SnapHelper)

protected:
	static void _bind_methods();

public:
	int find_target_snap_position(RecyclerView *p_recycler_view, float p_velocity) override;
	int calculate_time_for_scrolling(int p_dx) const override;

private:
	static constexpr int MAX_SCROLL_ON_FLING_DURATION = 100;
};

} // namespace godot
