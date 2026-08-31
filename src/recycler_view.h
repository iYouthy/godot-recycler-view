#pragma once

#include "adapter.h"
#include "adapter_helper.h"
#include "fling_scroller.h"
#include "item_animator.h"
#include "item_decoration.h"
#include "item_touch_helper.h"
#include "layout_manager.h"
#include "recycler.h"
#include "scroll_bar.h"
#include "scroll_listener.h"
#include "snap_helper.h"
#include "state.h"
#include "velocity_tracker.h"
#include "view_holder.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <map>

namespace godot {

// Port of RecyclerView (minimal hub). A Control that lays out items provided by
// an Adapter via a LayoutManager, recycling ViewHolders that scroll out of the
// visible area. Item controls are positioned absolutely and clipped.
class RecyclerView : public Control {
	GDCLASS(RecyclerView, Control)

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	RecyclerView();
	~RecyclerView() override;

	void _gui_input(const Ref<InputEvent> &p_event) override;
	void _process(double p_delta) override;
	void _draw() override;

	enum ScrollState {
		SCROLL_STATE_IDLE = 0,
		SCROLL_STATE_DRAGGING = 1,
		SCROLL_STATE_SETTLING = 2,
	};

	void set_adapter(const Ref<Adapter> &p_adapter);
	Ref<Adapter> get_adapter() const;

	void set_layout(const Ref<LayoutManager> &p_layout);
	Ref<LayoutManager> get_layout() const;

	Ref<State> get_state() const { return m_state; }
	Ref<Recycler> get_recycler() const { return m_recycler; }
	Ref<AdapterHelper> get_adapter_helper() const { return m_adapter_helper; }

	// Adapter-driven incremental updates (queue an op then re-layout).
	void notify_item_range_changed(int p_position, int p_count, const Variant &p_payload);
	void notify_item_range_inserted(int p_position, int p_count);
	void notify_item_range_removed(int p_position, int p_count);
	void notify_item_moved(int p_from_position, int p_to_position);
	// Full data-set change (notify_data_set_changed): clears queued updates and
	// re-lays out everything.
	void notify_data_changed();

	// Item view plumbing used by LayoutManagers.
	Ref<ViewHolder> get_view_for_position(int p_position);
	void recycle_view(const Ref<ViewHolder> &p_holder, int p_position);
	void add_item_view(const Ref<ViewHolder> &p_holder);
	void remove_item_view(const Ref<ViewHolder> &p_holder);
	void set_item_view_position(const Ref<ViewHolder> &p_holder, const Vector2 &p_pos, const Vector2 &p_size);
	int get_child_holder_count() const { return m_children.size(); }
	Ref<ViewHolder> get_child_holder_at(int p_index) const;

	int get_scroll_offset() const { return m_scroll_offset; }
	int get_scroll_offset_horizontal() const { return m_scroll_offset_h; }
	void set_scroll_offset(int p_offset);
	void set_scroll_offset_horizontal(int p_offset);
	// Scrolls by a delta and returns the amount actually scrolled (0 when the
	// layout cannot scroll or the offset is clamped). Dispatches on_scrolled.
	int scroll_vertically(int p_delta);
	int scroll_horizontally(int p_delta);
	void scroll_along_axis(int p_delta);
	Vector2 get_viewport_size() const;
	// Smoothly scrolls the layout's primary axis to the given offset over
	// p_duration seconds with a decelerating ease (used by SnapHelper to settle
	// on a snapped item/page).
	void smooth_scroll_to(int p_target, double p_duration);
	// Scrolls so the item at the given position is visible: top-aligned for a
	// normal layout, bottom-aligned (mirroring Android's reverseLayout) when the
	// layout reverses. Jumps immediately.
	void scroll_to_position(int p_position);
	// Smoothly scrolls to the item at the position (same target as
	// scroll_to_position) over p_duration seconds.
	void smooth_scroll_to_position(int p_position, double p_duration);
	// Snap helper (LinearSnapHelper center-snap / PagerSnapHelper page-snap).
	// Set it via SnapHelper::attach_to_recycler_view().
	void set_snap_helper(const Ref<SnapHelper> &p_helper);
	Ref<SnapHelper> get_snap_helper() const { return m_snap_helper; }

	// Scroll state machine (SCROLL_STATE_IDLE/DRAGGING/SETTLING) and the
	// listener callbacks, mirroring RecyclerView.OnScrollListener.
	int get_scroll_state() const { return m_scroll_state; }
	// Aborts a running fling and returns to IDLE.
	void stop_scroll();
	void add_on_scroll_listener(const Ref<ScrollListener> &p_listener);
	void remove_on_scroll_listener(const Ref<ScrollListener> &p_listener);
	void clear_on_scroll_listeners();

	// Item animations after incremental updates (insert/remove/move/change).
	void set_item_animator(const Ref<ItemAnimator> &p_animator);
	Ref<ItemAnimator> get_item_animator() const { return m_item_animator; }

	// Scroll bar (protocol: RecyclerViewScrollBar, default DefaultScrollBar). Attached as a
	// child Control pinned to the trailing edge; the RV notifies it on every
	// scroll/layout. The bar is owned by the RV's node tree (add_child), so it
	// must NOT be freed manually. Pass null to remove the bar.
	void set_scroll_bar(RecyclerViewScrollBar *p_bar);
	RecyclerViewScrollBar *get_scroll_bar() const { return m_scroll_bar; }
	// Whether the attached scroll bar fades out while the RV sits idle
	// (forwarded to the bar; default true, adjustable in the inspector).
	void set_scroll_bar_auto_hide(bool p_enabled);
	bool get_scroll_bar_auto_hide() const { return m_scroll_bar_auto_hide; }
	// Seconds the bar stays visible after the last activity before auto-hiding
	// (forwarded to the bar; default 0.5, Android's default fade delay).
	void set_scroll_bar_hide_delay(float p_delay);
	float get_scroll_bar_hide_delay() const { return m_scroll_bar_hide_delay; }

	// Item touch helper (long-press drag-reorder / swipe-dismiss). Set it via
	// ItemTouchHelper::attach_to_recycler_view().
	void set_item_touch_helper(const Ref<ItemTouchHelper> &p_helper);
	Ref<ItemTouchHelper> get_item_touch_helper() const { return m_item_touch_helper; }
	// Topmost child holder whose layout slot (get_layout_position) contains the
	// RV-local point.
	Ref<ViewHolder> find_child_holder_at(const Vector2 &p_local_pos);
	// True while the touch helper is dragging/swiping or settling the holder:
	// the layout and the ItemAnimator must leave it alone.
	bool is_item_touch_occupied(const Ref<ViewHolder> &p_holder) const;
	// Port of Adapter.onFailedToRecycleView: consults the adapter before a
	// non-recyclable holder (set_is_recyclable(false)) is recycled. Returns true
	// to force the recycle; false (default) keeps the holder attached and the
	// decision is re-visited on later layout passes.
	bool on_failed_to_recycle_view(const Ref<ViewHolder> &p_holder);
	// True once a drag has actually started scrolling (past the slop).
	bool is_scroll_drag_active() const { return m_dragging && m_drag_scrolled; }
	// True while a deferred layout (notify_*) is still pending this frame.
	bool is_layout_requested() const { return m_layout_deferred; }
	// The animator re-queries a holder's layout target here each frame so a
	// move slides to the live layout position (it follows scrolls).
	Vector2 get_layout_position(const Ref<ViewHolder> &p_holder);
	// Called by the animator when a holder's last animation finishes: if the
	// holder's layout slot is fully outside the viewport it is recycled right
	// away. Without this, an item that scrolled out while animating would only
	// be reclaimed on the next layout pass — during rapid scrolling every
	// animation period then fabricates a fresh view into an empty pool.
	void recycle_if_out_of_view(const Ref<ViewHolder> &p_holder);
	// Called by the animator when a removed holder finished fading out.
	void recycle_removed(const Ref<ViewHolder> &p_holder);

	// In a horizontal layout, whether the vertical mouse wheel also drives
	// horizontal scrolling (default true). When false, only WHEEL_LEFT/RIGHT
	// (touchpad swipe, Shift+wheel) scroll horizontally.
	void set_vertical_wheel_scrolls_horizontal(bool p_enabled);
	bool get_vertical_wheel_scrolls_horizontal() const { return m_vertical_wheel_scrolls_horizontal; }

	void set_item_extent(int p_size);
	int get_default_item_extent() const { return m_item_extent; }
	// Extent of the item at the given position along the scroll axis: the
	// adapter's variable extent, or the default item extent when not provided.
	int get_item_extent(int p_position) const;

	// Content-driven item sizes (off by default). When enabled, each item's
	// extent along the scroll axis is measured from the item control itself
	// instead of the static item extent: the root control's combined minimum
	// size decides the slot (Android's wrap_content), clamped by its combined
	// maximum size when one is declared, and a root control with SIZE_EXPAND
	// along the scroll axis fills the viewport (Android's match_parent).
	// Measured extents are cached by position and re-measured after any data
	// change. While disabled the static extent path is completely unchanged.
	void set_auto_measure_items(bool p_enabled);
	bool get_auto_measure_items() const { return m_auto_measure_items; }

	// Prefetch toggle (default on): after a layout, holders for the positions
	// just past the viewport are pre-created into the recycled pool.
	void set_prefetch_enabled(bool p_enabled);
	bool get_prefetch_enabled() const { return m_prefetch_enabled; }

	// Drag plumbing used by nested RecyclerViews: when a child hands a
	// perpendicular drag off to an ancestor RV (its axis isn't the dominant
	// one), the child becomes a conduit that forwards motion/release here.
	void begin_drag(const Ref<InputEventMouseMotion> &p_mm);
	void continue_drag(const Ref<InputEventMouseMotion> &p_mm);
	void end_drag();
	// Ends a drag without flinging: used when a release outside the window is
	// only detected later by a motion with the left button up (the velocity
	// would be stale), and by the ItemTouchHelper when it steals a gesture.
	void cancel_drag();

	// Item decorations (dividers, spacing).
	void add_item_decoration(const Ref<ItemDecoration> &p_decor);
	void remove_item_decoration(const Ref<ItemDecoration> &p_decor);
	int get_item_decoration_count() const { return m_decorations.size(); }
	// Accumulated (left, top, right, bottom) insets for the position.
	Vector4 get_item_insets(int p_position) const;
	// The position's actual drawn rect after insets are applied.
	Rect2 get_decorated_item_rect(int p_position) const;

	void layout_children();
	void request_layout();
	// Notify paths do not re-layout synchronously: a single notify_* call is one
	// op of a larger dispatch (DiffUtil emits several per diff), and laying out
	// per-op would consume an intermediate op state against the already-updated
	// adapter, duplicating views. Instead each notify_* defers one layout to the
	// end of the frame (deduplicated), so all ops land in one consume pass.
	void defer_layout();
	// Frees every item Control (visible and recycled). Teardown helper.
	void free_items();

private:
	void detach_from_adapter();
	void attach_to_adapter();
	void process_pending_updates();
	void mark_data_changed();
	// Forwards a mouse event landing on the visible scroll bar to it (Godot's
	// GUI hit-test can pick the item views over the bar). Returns true if the
	// bar handled the event.
	bool forward_to_scroll_bar(const Ref<InputEvent> &p_event);

	// Scroll state machine internals.
	int get_max_scroll_offset();
	int get_max_scroll_offset_horizontal();
	// The scroll offset that brings the item at the position into view: its
	// leading edge top-aligned (normal) or its trailing edge bottom-aligned
	// (reverse layout). Clamped by set_scroll_offset.
	int target_offset_for_position(int p_position);
	void set_scroll_state(int p_state);
	void dispatch_scrolled(int p_dx, int p_dy);
	void finish_drag();
	void stop_fling();
	// Drives the smooth_scroll_to settle animation (decelerate ease on the
	// primary axis). Takes precedence over the fling while active.
	void advance_settle(double p_delta);

	// Item animation: two-phase layout. Captures each visible holder's position
	// before pending updates are consumed, then dispatches move/add/remove/
	// change animations after the post layout. FLAG_UPDATE is set during the
	// consume step (and cleared by the rebind), so the updated set is collected
	// there, not during the pre-capture.
	struct PrePosition {
		Ref<ViewHolder> holder;
		Vector2 position;
	};
	Vector<PrePosition> m_pre_positions;
	Vector<Ref<ViewHolder>> m_updated_holders;
	// Holders whose data was removed this cycle (distinct from holders merely
	// recycled out of the visible range by the layout).
	Vector<Ref<ViewHolder>> m_removed_holders;
	void capture_pre_positions();
	void dispatch_animations();
	bool in_pre_positions(const Ref<ViewHolder> &p_holder) const;
	// True when the holder's layout rect lies entirely outside the viewport. Such
	// holders are recycled on the next pass; animating them here would only
	// re-trigger a move on every update (they never finish) and block recycling.
	bool is_holder_out_of_view(const Ref<ViewHolder> &p_holder) const;
	// Prefetch: after a layout, pre-creates holders for the positions just past
	// the viewport in the current scroll direction into the recycled pool.
	void prefetch_adjacent();
	// Prefetch + measure: with auto-measure on, creates/binds the holder for a
	// position just past the viewport, temporarily attaches it to measure at
	// the layout width, and returns it to the recycler. The measurement lands
	// in the extent cache before the row scrolls into view, so entering the
	// viewport does not change the offset table — a change there would move
	// every visible row (scrolling the other way visibly jitters).
	void prefetch_and_measure(int p_position);
	// First-mount bind: Godot runs an item scene's ready pass at the end of the
	// frame, not inside add_child, so a freshly created control's @onready
	// references are still null right after the mount. add_item_view connects
	// this one-shot handler to the control's ready signal for that first mount;
	// the deferred bind runs here, with the scene initialized.
	void _on_item_ready(const Ref<ViewHolder> &p_holder);
	bool try_start_fling(float p_velocity);

	// Auto-measure (Android wrap_content / match_parent): measures the item's
	// control at the width the layout assigns it and caches the result by
	// position (0 = not measured, sized to the item count). Entries are
	// invalidated narrowly: update ops shift the array (insert/remove/move)
	// and zero the changed range (update); full data-set changes and
	// decoration changes clear everything.
	int measure_item_extent(const Ref<ViewHolder> &p_holder, int p_position, float p_width);
	// Shifts the measured-extent array with the pending update ops: ADD
	// inserts zeros (new rows are unmeasured), REMOVE drops the range, MOVE
	// moves the value, UPDATE zeroes the range so the next layout re-measures
	// the changed rows only.
	void offset_measured_extents_for_ops(const Vector<UpdateOp> &p_ops);
	void clear_measured_extents();
	// Presets the cross-axis size of a control and, recursively, of its
	// children (fill semantics: children span their container's width), and
	// invalidates each control's minimum-size cache. Used by auto-measure so
	// width-sensitive content (fit_content text) shapes at the final layout
	// width even inside containers that have not been laid out yet — without
	// it, a RichTextLabel inside a VBox would shape at its initial 0 width and
	// report one line per character.
	void preset_item_cross_size(Control *p_control, float p_cross, bool p_vertical);
	// Re-anchors the scroll offset to the pending scroll target once the
	// measured extents settle (port of Android's mPendingScrollPosition).
	// Returns true when the offset moved (the layout should run one more pass).
	bool correct_pending_scroll_target();
	bool m_auto_measure_items = false;
	Vector<int> m_measured_extents;
	// Parallel to m_measured_extents: true where the measured extent came from
	// a match_parent (SIZE_EXPAND) row. Only those depend on the viewport
	// height, so a pure height change invalidates just them, while wrap_content
	// rows keep their measurements (their height depends on the width only).
	Vector<bool> m_measured_expand_flags;
	// Last size seen by NOTIFICATION_RESIZED, to distinguish width changes
	// (every row's wrap width changes -> remeasure everything) from pure
	// height changes (only match_parent rows are viewport-derived).
	Size2 m_last_resize_size = Size2(-1.0f, -1.0f);
	// Adaptive per-view-type estimates for unmeasured rows: the running mean
	// of measured wrap_content extents for that view type. The mean (not an
	// EMA) is what keeps the estimate stable for alternating row heights —
	// an EMA oscillates between the two heights forever, and every wiggle
	// moves the offset table of the unmeasured rows (and the scroll bar's
	// thumb) with it. Unmeasured rows fall back to this instead of the fixed
	// item_extent, so a row entering the viewport changes the table as little
	// as possible. match_parent rows never update it (they would pollute it
	// with the viewport size).
	struct ExtentEstimate {
		int sum = 0;
		int count = 0;
		int value = 0;
		// True once a match_parent (SIZE_EXPAND) row measured for this view
		// type: such rows are viewport-derived, so their unmeasured estimate
		// is the live viewport size (see get_item_extent), not the mean.
		bool is_expand = false;
	};
	std::map<int, ExtentEstimate> m_extent_estimates;
	// Scroll target set by scroll_to_position / smooth_scroll_to_position and
	// re-anchored after the measured extents settle; -1 when nothing is pending.
	int m_pending_scroll_target = -1;
	// The re-anchor's target offset from the previous correction pass. The
	// target is only considered reached once it is both in place and unchanged
	// across two passes: measured extents refine while rows enter the
	// viewport, so a single-pass match can be against a table that changes a
	// moment later (the list would then jump again as the estimate refines).
	int m_last_correct_raw = -1;
	// Bounded re-runs for the auto-measure convergence loop.
	static constexpr int MAX_AUTO_MEASURE_PASSES = 4;

	// Nested scroll: cascade forwarding and ancestor lookup.
	RecyclerView *find_ancestor_recycler_view() const;
	RecyclerView *find_ancestor_scrolling_axis(bool p_horizontal) const;
	bool has_ancestor_scrolling_axis(bool p_horizontal) const;
	void forward_vertical_scroll(int p_delta);
	void forward_horizontal_scroll(int p_delta);
	void forward_scroll_to_ancestor(int p_delta, bool p_horizontal);
	void start_nested_fling(float p_velocity);
	// Forwards a drag event to the RV that grabbed it, converting the event's
	// position from this RV's local space into the receiver's.
	void forward_event_to(RecyclerView *p_target, const Ref<InputEvent> &p_event);
	bool forward_unowned_drag_event(const Ref<InputEvent> &p_event);

	// Drag grab: when this RV owns a drag, ancestors route unowned drag events
	// back to it, mirroring Android's sticky touch capture (Godot re-hit-tests
	// every mouse event, so a drag leaving the RV would otherwise orphan it).
	void set_drag_grabber(RecyclerView *p_grabber) { m_drag_grabber = p_grabber; }
	void begin_drag_grabber_chain();
	void clear_drag_grabber_chain();

	Ref<Adapter> m_adapter;
	Ref<LayoutManager> m_layout;
	Ref<Recycler> m_recycler;
	Ref<State> m_state;
	Ref<AdapterDataObserver> m_data_observer;
	Ref<AdapterHelper> m_adapter_helper;
	Vector<Ref<ItemDecoration>> m_decorations;
	Ref<ItemAnimator> m_item_animator;
	Ref<ItemTouchHelper> m_item_touch_helper;
	Ref<SnapHelper> m_snap_helper;
	RecyclerViewScrollBar *m_scroll_bar = nullptr;
	bool m_scroll_bar_auto_hide = true;
	float m_scroll_bar_hide_delay = 0.5f;
	// True while a drag started on the scroll bar is active: the RV keeps
	// routing mouse events to it even when the thumb is dragged outside the
	// bar's narrow strip.
	bool m_scroll_bar_dragging = false;
	bool m_layout_deferred = false;
	// Set when a layout request arrives while a layout is already running (e.g.
	// a RESIZED notification for the size being finalized). The running layout
	// re-runs once afterwards so a size/state change is never dropped.
	bool m_layout_requested_again = false;

	// Tracked child ViewHolders (in tree order), for recycling on scroll.
	Vector<Ref<ViewHolder>> m_children;

	int m_scroll_offset = 0;
	int m_scroll_offset_h = 0;
	// Direction of the most recent scroll (-1 up/left, 0 none, +1 down/right).
	// Drives the prefetch so it pre-creates the runway on the correct side.
	int m_last_scroll_direction = 0;
	bool m_prefetch_enabled = true;
	// Item extent along the scroll axis, used by the LayoutManager. In a full port
	// this is derived from the adapter/measurement; fixed for the first slice.
	int m_item_extent = 64;
	bool m_layout_in_progress = false;

	// Scroll state machine (Android's SCROLL_STATE_IDLE/DRAGGING/SETTLING).
	int m_scroll_state = SCROLL_STATE_IDLE;
	Vector<Ref<ScrollListener>> m_scroll_listeners;
	VelocityTracker m_velocity_tracker_v;
	VelocityTracker m_velocity_tracker_h;
	FlingScroller m_fling_v;
	FlingScroller m_fling_h;
	// smooth_scroll_to settle animation state.
	bool m_settle_active = false;
	int m_settle_from = 0;
	int m_settle_to = 0;
	double m_settle_elapsed = 0.0;
	double m_settle_duration = 0.0;
	// Monotonic clock (ms) accumulated in _process, used to stamp drag samples.
	double m_elapsed_ms = 0.0;
	static constexpr float MIN_FLING_VELOCITY = 50.0f;

	bool m_dragging = false;
	int m_drag_start_mouse = 0;
	int m_drag_start_mouse_x = 0;
	int m_drag_start_scroll = 0;
	int m_drag_start_scroll_h = 0;
	bool m_drag_scrolled = false;
	// Nested drag: when the dominant axis isn't this RV's and an ancestor
	// scrolls it, the gesture is handed off and this RV becomes a conduit.
	bool m_drag_handed_off = false;
	RecyclerView *m_drag_handoff_target = nullptr;
	// Descendant RV currently owning a drag; this RV forwards unowned drag
	// events to it so the gesture survives the mouse leaving the child.
	RecyclerView *m_drag_grabber = nullptr;
	bool m_vertical_wheel_scrolls_horizontal = true;
};

} // namespace godot
