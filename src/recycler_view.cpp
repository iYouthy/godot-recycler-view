#include "recycler_view.h"

#include <godot_cpp/classes/input_event_mouse.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

// Internal adapter observer that triggers an incremental re-layout on data
// changes via the AdapterHelper.
class RecyclerViewDataObserver : public AdapterDataObserver {
	RecyclerView *m_rv = nullptr;

public:
	void set_recycler_view(RecyclerView *p_rv) { m_rv = p_rv; }

	void on_changed() override {
		if (m_rv) {
			m_rv->notify_data_changed();
		}
	}

	void on_item_range_changed(int p_position, int p_count, const Variant &p_payload) override {
		if (m_rv) {
			m_rv->notify_item_range_changed(p_position, p_count, p_payload);
		}
	}

	void on_item_range_inserted(int p_position, int p_count) override {
		if (m_rv) {
			m_rv->notify_item_range_inserted(p_position, p_count);
		}
	}

	void on_item_range_removed(int p_position, int p_count) override {
		if (m_rv) {
			m_rv->notify_item_range_removed(p_position, p_count);
		}
	}

	void on_item_moved(int p_from_position, int p_to_position) override {
		if (m_rv) {
			m_rv->notify_item_moved(p_from_position, p_to_position);
		}
	}
};

void RecyclerView::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_adapter", "adapter"), &RecyclerView::set_adapter);
	ClassDB::bind_method(D_METHOD("get_adapter"), &RecyclerView::get_adapter);
	ClassDB::bind_method(D_METHOD("set_layout", "layout"), &RecyclerView::set_layout);
	ClassDB::bind_method(D_METHOD("get_layout"), &RecyclerView::get_layout);
	ClassDB::bind_method(D_METHOD("get_state"), &RecyclerView::get_state);
	ClassDB::bind_method(D_METHOD("get_recycler"), &RecyclerView::get_recycler);
	ClassDB::bind_method(D_METHOD("get_adapter_helper"), &RecyclerView::get_adapter_helper);
	ClassDB::bind_method(D_METHOD("notify_item_range_changed", "position", "count", "payload"), &RecyclerView::notify_item_range_changed);
	ClassDB::bind_method(D_METHOD("notify_item_range_inserted", "position", "count"), &RecyclerView::notify_item_range_inserted);
	ClassDB::bind_method(D_METHOD("notify_item_range_removed", "position", "count"), &RecyclerView::notify_item_range_removed);
	ClassDB::bind_method(D_METHOD("notify_item_moved", "from_position", "to_position"), &RecyclerView::notify_item_moved);
	ClassDB::bind_method(D_METHOD("notify_data_changed"), &RecyclerView::notify_data_changed);
	ClassDB::bind_method(D_METHOD("set_scroll_offset", "offset"), &RecyclerView::set_scroll_offset);
	ClassDB::bind_method(D_METHOD("get_scroll_offset"), &RecyclerView::get_scroll_offset);
	ClassDB::bind_method(D_METHOD("set_scroll_offset_horizontal", "offset"), &RecyclerView::set_scroll_offset_horizontal);
	ClassDB::bind_method(D_METHOD("get_scroll_offset_horizontal"), &RecyclerView::get_scroll_offset_horizontal);
	ClassDB::bind_method(D_METHOD("scroll_vertically", "delta"), &RecyclerView::scroll_vertically);
	ClassDB::bind_method(D_METHOD("scroll_horizontally", "delta"), &RecyclerView::scroll_horizontally);
	ClassDB::bind_method(D_METHOD("get_scroll_state"), &RecyclerView::get_scroll_state);
	ClassDB::bind_method(D_METHOD("stop_scroll"), &RecyclerView::stop_scroll);
	ClassDB::bind_method(D_METHOD("add_on_scroll_listener", "listener"), &RecyclerView::add_on_scroll_listener);
	ClassDB::bind_method(D_METHOD("remove_on_scroll_listener", "listener"), &RecyclerView::remove_on_scroll_listener);
	ClassDB::bind_method(D_METHOD("clear_on_scroll_listeners"), &RecyclerView::clear_on_scroll_listeners);
	ClassDB::bind_method(D_METHOD("set_vertical_wheel_scrolls_horizontal", "enabled"), &RecyclerView::set_vertical_wheel_scrolls_horizontal);
	ClassDB::bind_method(D_METHOD("get_vertical_wheel_scrolls_horizontal"), &RecyclerView::get_vertical_wheel_scrolls_horizontal);
	ClassDB::bind_method(D_METHOD("set_item_extent", "size"), &RecyclerView::set_item_extent);
	ClassDB::bind_method(D_METHOD("get_default_item_extent"), &RecyclerView::get_default_item_extent);
	ClassDB::bind_method(D_METHOD("set_auto_measure_items", "enabled"), &RecyclerView::set_auto_measure_items);
	ClassDB::bind_method(D_METHOD("get_auto_measure_items"), &RecyclerView::get_auto_measure_items);
	ClassDB::bind_method(D_METHOD("set_prefetch_enabled", "enabled"), &RecyclerView::set_prefetch_enabled);
	ClassDB::bind_method(D_METHOD("get_prefetch_enabled"), &RecyclerView::get_prefetch_enabled);
	ClassDB::bind_method(D_METHOD("get_item_extent", "position"), &RecyclerView::get_item_extent);
	ClassDB::bind_method(D_METHOD("layout_children"), &RecyclerView::layout_children);
	ClassDB::bind_method(D_METHOD("request_layout"), &RecyclerView::request_layout);
	ClassDB::bind_method(D_METHOD("free_items"), &RecyclerView::free_items);
	ClassDB::bind_method(D_METHOD("get_view_for_position", "position"), &RecyclerView::get_view_for_position);
	ClassDB::bind_method(D_METHOD("recycle_view", "holder", "position"), &RecyclerView::recycle_view);
	ClassDB::bind_method(D_METHOD("add_item_view", "holder"), &RecyclerView::add_item_view);
	ClassDB::bind_method(D_METHOD("remove_item_view", "holder"), &RecyclerView::remove_item_view);
	ClassDB::bind_method(D_METHOD("set_item_view_position", "holder", "position", "size"), &RecyclerView::set_item_view_position);
	ClassDB::bind_method(D_METHOD("get_child_holder_count"), &RecyclerView::get_child_holder_count);
	ClassDB::bind_method(D_METHOD("get_child_holder_at", "index"), &RecyclerView::get_child_holder_at);
	ClassDB::bind_method(D_METHOD("get_viewport_size"), &RecyclerView::get_viewport_size);
	ClassDB::bind_method(D_METHOD("add_item_decoration", "decor"), &RecyclerView::add_item_decoration);
	ClassDB::bind_method(D_METHOD("remove_item_decoration", "decor"), &RecyclerView::remove_item_decoration);
	ClassDB::bind_method(D_METHOD("get_item_decoration_count"), &RecyclerView::get_item_decoration_count);
	ClassDB::bind_method(D_METHOD("get_item_insets", "position"), &RecyclerView::get_item_insets);
	ClassDB::bind_method(D_METHOD("get_decorated_item_rect", "position"), &RecyclerView::get_decorated_item_rect);
	ClassDB::bind_method(D_METHOD("set_item_animator", "animator"), &RecyclerView::set_item_animator);
	ClassDB::bind_method(D_METHOD("get_item_animator"), &RecyclerView::get_item_animator);
	ClassDB::bind_method(D_METHOD("set_item_touch_helper", "helper"), &RecyclerView::set_item_touch_helper);
	ClassDB::bind_method(D_METHOD("get_item_touch_helper"), &RecyclerView::get_item_touch_helper);
	ClassDB::bind_method(D_METHOD("find_child_holder_at", "local_pos"), &RecyclerView::find_child_holder_at);
	ClassDB::bind_method(D_METHOD("is_item_touch_occupied", "holder"), &RecyclerView::is_item_touch_occupied);
	ClassDB::bind_method(D_METHOD("smooth_scroll_to", "target", "duration"), &RecyclerView::smooth_scroll_to);
	ClassDB::bind_method(D_METHOD("scroll_to_position", "position"), &RecyclerView::scroll_to_position);
	ClassDB::bind_method(D_METHOD("smooth_scroll_to_position", "position", "duration"), &RecyclerView::smooth_scroll_to_position);
	ClassDB::bind_method(D_METHOD("set_snap_helper", "helper"), &RecyclerView::set_snap_helper);
	ClassDB::bind_method(D_METHOD("get_snap_helper"), &RecyclerView::get_snap_helper);
	ClassDB::bind_method(D_METHOD("set_scroll_bar", "bar"), &RecyclerView::set_scroll_bar);
	ClassDB::bind_method(D_METHOD("get_scroll_bar"), &RecyclerView::get_scroll_bar);
	ClassDB::bind_method(D_METHOD("set_scroll_bar_auto_hide", "enabled"), &RecyclerView::set_scroll_bar_auto_hide);
	ClassDB::bind_method(D_METHOD("get_scroll_bar_auto_hide"), &RecyclerView::get_scroll_bar_auto_hide);
	ClassDB::bind_method(D_METHOD("set_scroll_bar_hide_delay", "delay"), &RecyclerView::set_scroll_bar_hide_delay);
	ClassDB::bind_method(D_METHOD("get_scroll_bar_hide_delay"), &RecyclerView::get_scroll_bar_hide_delay);

	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_IDLE", SCROLL_STATE_IDLE);
	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_DRAGGING", SCROLL_STATE_DRAGGING);
	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_SETTLING", SCROLL_STATE_SETTLING);

	// adapter/layout are runtime assembly, not scene data: they hold RefCounted
	// objects (not Resources) that the scene saver cannot serialize. Keep them
	// visible in the inspector but never persist them (PROPERTY_USAGE_EDITOR
	// omits PROPERTY_USAGE_STORAGE), or a @tool scene that sets them breaks
	// scene saving with "Resource was not pre cached for the resource section".
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "adapter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_adapter", "get_adapter");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layout", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_layout", "get_layout");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_extent"), "set_item_extent", "get_default_item_extent");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_measure_items"), "set_auto_measure_items", "get_auto_measure_items");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vertical_wheel_scrolls_horizontal"), "set_vertical_wheel_scrolls_horizontal", "get_vertical_wheel_scrolls_horizontal");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "scroll_bar_auto_hide"), "set_scroll_bar_auto_hide", "get_scroll_bar_auto_hide");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scroll_bar_hide_delay"), "set_scroll_bar_hide_delay", "get_scroll_bar_hide_delay");
}

RecyclerView::RecyclerView() {
	set_clip_contents(true);
	m_recycler.instantiate();
	m_state.instantiate();
	m_adapter_helper.instantiate();
	Ref<RecyclerViewDataObserver> observer;
	observer.instantiate();
	observer->set_recycler_view(this);
	m_data_observer = observer;
}

RecyclerView::~RecyclerView() {
	if (m_item_animator.is_valid()) {
		m_item_animator->clear();
	}
	if (m_item_touch_helper.is_valid()) {
		m_item_touch_helper->on_recycler_view_destroyed();
	}
	if (m_snap_helper.is_valid()) {
		m_snap_helper->on_recycler_view_destroyed();
	}
	detach_from_adapter();
	// Cached/scrap/pool views are detached from this RV (removed from the tree),
	// so the Node teardown never deletes them. Free them here or nested RVs that
	// are deleted without an explicit free_items() would leak.
	m_recycler->free_all_views();
}

void RecyclerView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			// _process drives the fling and the drag-sample clock.
			set_process(true);
			if (m_auto_measure_items) {
				// A layout that ran before this RV entered the tree measured
				// items off-tree (min/max cache invalidation is a no-op until
				// then) and kept the estimates. Re-layout now so the tree-side
				// measurements take over.
				defer_layout();
			}
			break;
		case NOTIFICATION_RESIZED: {
			// Auto-measure: a width change re-wraps every row, so all cached
			// measurements go stale. A pure height change does not touch
			// wrap_content rows (their height depends on the width only) — only
			// match_parent rows are viewport-derived and must re-measure.
			// Keeping the wrap_content measurements is what stops the list
			// from re-measuring everything (and visibly jittering) while it
			// scrolls after a resize.
			const Size2 size = get_size();
			const bool width_changed = size.x != m_last_resize_size.x;
			const bool height_changed = size.y != m_last_resize_size.y;
			m_last_resize_size = size;
			if (m_auto_measure_items && (width_changed || height_changed)) {
				if (width_changed) {
					clear_measured_extents();
				} else if (!m_measured_expand_flags.is_empty()) {
					for (int i = 0; i < m_measured_extents.size(); i++) {
						if (m_measured_expand_flags[i]) {
							m_measured_extents.write[i] = 0;
						}
					}
				}
				// Dropped measurements must rebuild the offset table. Without
				// this, a layout that ran before (e.g. the previous frame of a
				// window drag) left the table clean, the table keeps the stale
				// values, and the re-measured rows overflow their slots: with
				// a growing viewport, a match_parent row measures taller than
				// the stale table says and overlaps the row below it.
				// Guard: a scene node can hit RESIZED (container layout) before
				// set_layout ran — the loader crash this guard prevents.
				if (m_layout.is_valid()) {
					m_layout->on_data_changed();
				}
			}
			layout_children();
			break;
		}
	}
}

void RecyclerView::_gui_input(const Ref<InputEvent> &p_event) {
	// Route clicks/drags on the visible scroll bar to it before anything else:
	// Godot's GUI hit-test can pick the item views over the bar (clipping and
	// z-order are not considered the same way), so the RV — the fallback
	// receiver — forwards events that land on the bar instead of scrolling.
	if (forward_to_scroll_bar(p_event)) {
		return;
	}
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		// The touch helper owns an active gesture (long-press drag / swipe):
		// give it first crack so it can consume press/release and keep the RV
		// from scrolling or flinging while it is in control.
		if (m_item_touch_helper.is_valid() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				if (m_item_touch_helper->on_press(mb, this)) {
					accept_event();
					return;
				}
			} else if (m_item_touch_helper->on_release(mb, this)) {
				accept_event();
				return;
			}
		}
		// Route the drag release to the RV that grabbed the gesture, so it ends
		// cleanly even when the mouse left the child's bounds.
		if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT
				&& !mb->is_pressed() && forward_unowned_drag_event(p_event)) {
			return;
		}
		if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				stop_fling();
				m_velocity_tracker_v.clear();
				m_velocity_tracker_h.clear();
				m_dragging = true;
				m_drag_handed_off = false;
				m_drag_handoff_target = nullptr;
				m_drag_start_mouse = (int)mb->get_position().y;
				m_drag_start_mouse_x = (int)mb->get_position().x;
				m_drag_start_scroll = m_scroll_offset;
				m_drag_start_scroll_h = m_scroll_offset_h;
				m_drag_scrolled = false;
				set_scroll_state(SCROLL_STATE_DRAGGING);
				begin_drag_grabber_chain();
			} else if (m_dragging) {
				if (m_drag_handed_off) {
					m_drag_handoff_target->end_drag();
					m_drag_handed_off = false;
					m_drag_handoff_target = nullptr;
					m_dragging = false;
				} else {
					finish_drag();
				}
			}
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_UP) {
			forward_vertical_scroll(-(int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_DOWN) {
			forward_vertical_scroll((int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_LEFT) {
			forward_horizontal_scroll(-(int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_RIGHT) {
			forward_horizontal_scroll((int)(mb->get_factor() * 48.0f));
			accept_event();
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		// The touch helper consumes motion while it owns a gesture (drag/swipe),
		// so the RV never starts scrolling underneath it.
		if (m_item_touch_helper.is_valid() && m_item_touch_helper->on_motion(mm, this)) {
			accept_event();
			return;
		}
		// Route the drag motion to the RV that grabbed it (this RV's grabber,
		// or any ancestor's), converting its position into the receiver's space.
		if (forward_unowned_drag_event(p_event)) {
			return;
		}
		// A release outside the window never reaches _gui_input as a button
		// event, so the drag must be re-validated against the button mask on
		// every motion: with the left button up, the gesture was interrupted.
		// Unlike a real release it must NOT fling (the velocity would be stale).
		if (!mm->get_button_mask().has_flag(MouseButtonMask::MOUSE_BUTTON_MASK_LEFT)) {
			if (m_dragging) {
				if (m_drag_handed_off) {
					m_drag_handoff_target->cancel_drag();
					m_drag_handed_off = false;
					m_drag_handoff_target = nullptr;
					m_dragging = false;
				} else {
					cancel_drag();
				}
			}
			return;
		}
		if (m_dragging) {
			if (m_drag_handed_off) {
				// Conduit: the drag belongs to an ancestor, keep routing to it
				// with the event converted into the ancestor's local space.
				forward_event_to(m_drag_handoff_target, p_event);
				return;
			}
			if (!m_drag_scrolled) {
				// Past the slop, decide who owns the drag by dominant axis.
				const int dx = (int)mm->get_position().x - m_drag_start_mouse_x;
				const int dy = (int)mm->get_position().y - m_drag_start_mouse;
				if (dx < -8 || dx > 8 || dy < -8 || dy > 8) {
					const bool dominant_horizontal = Math::abs(dx) > Math::abs(dy);
					const bool my_axis_horizontal = m_layout.is_valid() && m_layout->can_scroll_horizontally();
					if (dominant_horizontal != my_axis_horizontal) {
						RecyclerView *target = find_ancestor_scrolling_axis(dominant_horizontal);
						if (target != nullptr) {
							clear_drag_grabber_chain();
							mm->set_position(mm->get_position() + get_global_position() - target->get_global_position());
							target->begin_drag(mm);
							m_drag_handed_off = true;
							m_drag_handoff_target = target;
							set_scroll_state(SCROLL_STATE_IDLE);
							return;
						}
					}
					m_drag_scrolled = true;
				}
			}
			continue_drag(mm);
		}
	}
}

// Routes a mouse event that lands on the (visible) scroll bar to it, in the
// bar's local space. Godot's GUI hit-test can pick the item views over the bar,
// so the RV — the fallback receiver — forwards instead of scrolling. A hidden
// bar (auto-hide faded out) sets MOUSE_FILTER_IGNORE and is skipped, letting the
// click pass through to the RV.
bool RecyclerView::forward_to_scroll_bar(const Ref<InputEvent> &p_event) {
	if (m_scroll_bar == nullptr) {
		return false;
	}
	// A hidden bar (auto-hide faded out) is skipped; a drag that already started
	// keeps routing regardless of position (the thumb can leave the narrow bar).
	if (m_scroll_bar->get_mouse_filter() == MOUSE_FILTER_IGNORE && !m_scroll_bar_dragging) {
		return false;
	}
	const Ref<InputEventMouse> mouse = p_event;
	if (mouse.is_valid()) {
		const Rect2 bar_rect = m_scroll_bar->get_global_rect();
		const Vector2 global = mouse->get_position() + get_global_position();
		if (m_scroll_bar_dragging || bar_rect.has_point(global)) {
			mouse->set_position(global - bar_rect.position);
			m_scroll_bar->_gui_input(p_event);
			const Ref<InputEventMouseButton> mb = p_event;
			if (mb.is_valid() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
				m_scroll_bar_dragging = mb->is_pressed();
			} else {
				// A motion without the left button: the drag ended without a
				// release reaching us, stop routing bar events.
				const Ref<InputEventMouseMotion> mm = p_event;
				if (mm.is_valid() && !mm->get_button_mask().has_flag(MouseButtonMask::MOUSE_BUTTON_MASK_LEFT)) {
					m_scroll_bar_dragging = false;
				}
			}
			return true;
		}
	}
	return false;
}

// Scrolls along the layout's scroll axis. In a horizontal layout the vertical
// mouse wheel drives horizontal scrolling unless the user opted out, in which
// case only WHEEL_LEFT/RIGHT scroll horizontally.
void RecyclerView::scroll_along_axis(int p_delta) {
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		if (m_vertical_wheel_scrolls_horizontal) {
			scroll_horizontally(p_delta);
		}
	} else {
		scroll_vertically(p_delta);
	}
}

void RecyclerView::set_vertical_wheel_scrolls_horizontal(bool p_enabled) {
	m_vertical_wheel_scrolls_horizontal = p_enabled;
}

void RecyclerView::_process(double p_delta) {
	m_elapsed_ms += p_delta * 1000.0;
	if (m_item_touch_helper.is_valid()) {
		// Drives the long-press timer, drag edge auto-scroll and the recover
		// animations. Must run even when nothing else is animating.
		m_item_touch_helper->step(p_delta);
	}
	if (m_item_animator.is_valid() && m_item_animator->is_running()) {
		m_item_animator->animate_step(p_delta);
	}
	if (m_scroll_state != SCROLL_STATE_SETTLING) {
		return;
	}
	if (m_settle_active) {
		advance_settle(p_delta);
		return;
	}
	const double delta_ms = p_delta * 1000.0;
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		if (!m_fling_h.update(delta_ms)) {
			set_scroll_state(SCROLL_STATE_IDLE);
			return;
		}
		const int before = m_scroll_offset_h;
		set_scroll_offset_horizontal(m_fling_h.get_current_position());
		const int dx = m_scroll_offset_h - before;
		if (dx != 0) {
			dispatch_scrolled(dx, 0);
		}
	} else if (m_layout.is_valid() && m_layout->can_scroll_vertically()) {
		if (!m_fling_v.update(delta_ms)) {
			set_scroll_state(SCROLL_STATE_IDLE);
			return;
		}
		const int before = m_scroll_offset;
		set_scroll_offset(m_fling_v.get_current_position());
		const int dy = m_scroll_offset - before;
		if (dy != 0) {
			dispatch_scrolled(0, dy);
		}
	}
}

void RecyclerView::set_scroll_state(int p_state) {
	if (m_scroll_state == p_state) {
		return;
	}
	m_scroll_state = p_state;
	for (int i = 0; i < m_scroll_listeners.size(); i++) {
		m_scroll_listeners[i]->on_scroll_state_changed(p_state);
	}
}

void RecyclerView::dispatch_scrolled(int p_dx, int p_dy) {
	for (int i = 0; i < m_scroll_listeners.size(); i++) {
		m_scroll_listeners[i]->on_scrolled(p_dx, p_dy);
	}
}

void RecyclerView::stop_fling() {
	m_fling_v.stop();
	m_fling_h.stop();
}

void RecyclerView::stop_scroll() {
	stop_fling();
	if (m_scroll_state == SCROLL_STATE_SETTLING) {
		set_scroll_state(SCROLL_STATE_IDLE);
	}
}

bool RecyclerView::try_start_fling(float p_velocity) {
	// A snap helper may take over the fling (settle on a snapped item/page);
	// then the RV never runs its own inertial scroll. The helper receives the
	// effective fling velocity (the sign the fling would scroll with), matching
	// the -p_velocity negation inside start_nested_fling.
	if (m_snap_helper.is_valid() && m_snap_helper->on_fling(-p_velocity)) {
		return true;
	}
	if (m_layout.is_null()) {
		return false;
	}
	if (m_layout->can_scroll_horizontally()) {
		if (get_max_scroll_offset_horizontal() > 0) {
			start_nested_fling(p_velocity);
			return true;
		}
		RecyclerView *ancestor = find_ancestor_scrolling_axis(true);
		if (ancestor != nullptr) {
			ancestor->start_nested_fling(p_velocity);
			return true;
		}
		return false;
	}
	if (m_layout->can_scroll_vertically()) {
		if (get_max_scroll_offset() > 0) {
			start_nested_fling(p_velocity);
			return true;
		}
		RecyclerView *ancestor = find_ancestor_scrolling_axis(false);
		if (ancestor != nullptr) {
			ancestor->start_nested_fling(p_velocity);
			return true;
		}
		return false;
	}
	return false;
}

// Starts a fling on this RV's active axis. Momentum only spills to an ancestor
// when this RV is already pinned at an edge: a mid-list fling is consumed here
// first (reaching its own boundary), and only a release from the boundary lets
// the velocity carry into the ancestor. This mirrors Android, where a nested
// child reaches its end before the parent takes over, instead of both flying
// together and recycling the child's item mid-flight.
void RecyclerView::start_nested_fling(float p_velocity) {
	const bool rev = m_layout.is_valid() && m_layout->is_reverse_layout();
	// FlingScroller treats positive velocity as "offset increases". Reverse layout
	// shows older items as the offset decreases (content moves up), so flip the
	// velocity: an upward flick (negative) must drive the offset down.
	const float fling_velocity = rev ? -p_velocity : p_velocity;
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const int max_offset = get_max_scroll_offset_horizontal();
		m_fling_h.fling(m_scroll_offset_h, -fling_velocity, 0, max_offset);
		set_scroll_state(SCROLL_STATE_SETTLING);
		if (m_fling_h.was_clamped()
				&& (m_scroll_offset_h == 0 || m_scroll_offset_h == max_offset)) {
			RecyclerView *ancestor = find_ancestor_scrolling_axis(true);
			if (ancestor != nullptr) {
				ancestor->start_nested_fling(p_velocity);
			}
		}
	} else if (m_layout.is_valid() && m_layout->can_scroll_vertically()) {
		const int max_offset = get_max_scroll_offset();
		m_fling_v.fling(m_scroll_offset, -fling_velocity, 0, max_offset);
		set_scroll_state(SCROLL_STATE_SETTLING);
		if (m_fling_v.was_clamped()
				&& (m_scroll_offset == 0 || m_scroll_offset == max_offset)) {
			RecyclerView *ancestor = find_ancestor_scrolling_axis(false);
			if (ancestor != nullptr) {
				ancestor->start_nested_fling(p_velocity);
			}
		}
	}
}

void RecyclerView::finish_drag() {
	m_dragging = false;
	clear_drag_grabber_chain();
	if (!m_drag_scrolled) {
		m_drag_scrolled = false;
		set_scroll_state(SCROLL_STATE_IDLE);
		return;
	}
	m_drag_scrolled = false;
	float velocity = 0.0f;
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		velocity = m_velocity_tracker_h.get_velocity();
	} else {
		velocity = m_velocity_tracker_v.get_velocity();
	}
	if (velocity >= -MIN_FLING_VELOCITY && velocity <= MIN_FLING_VELOCITY) {
		set_scroll_state(SCROLL_STATE_IDLE);
		return;
	}
	if (!try_start_fling(velocity)) {
		set_scroll_state(SCROLL_STATE_IDLE);
	}
}

void RecyclerView::cancel_drag() {
	m_dragging = false;
	clear_drag_grabber_chain();
	m_drag_scrolled = false;
	m_velocity_tracker_v.clear();
	m_velocity_tracker_h.clear();
	set_scroll_state(SCROLL_STATE_IDLE);
}

// Drag plumbing used by nested scroll: an ancestor takes ownership of a drag
// handed off by a perpendicular child (see _gui_input).
void RecyclerView::begin_drag(const Ref<InputEventMouseMotion> &p_mm) {
	stop_fling();
	m_velocity_tracker_v.clear();
	m_velocity_tracker_h.clear();
	m_dragging = true;
	m_drag_handed_off = false;
	m_drag_handoff_target = nullptr;
	m_drag_start_mouse = (int)p_mm->get_position().y;
	m_drag_start_mouse_x = (int)p_mm->get_position().x;
	m_drag_start_scroll = m_scroll_offset;
	m_drag_start_scroll_h = m_scroll_offset_h;
	m_drag_scrolled = false;
	set_scroll_state(SCROLL_STATE_DRAGGING);
	begin_drag_grabber_chain();
}

void RecyclerView::continue_drag(const Ref<InputEventMouseMotion> &p_mm) {
	const bool rev = m_layout.is_valid() && m_layout->is_reverse_layout();
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const float x = p_mm->get_position().x;
		m_velocity_tracker_h.add_sample(x, m_elapsed_ms);
		int dx = (int)x - m_drag_start_mouse_x;
		if (!m_drag_scrolled && (dx < -8 || dx > 8)) {
			m_drag_scrolled = true;
		}
		if (m_drag_scrolled) {
			const int before = m_scroll_offset_h;
			// Reverse layout: scrolling the content down shows older items, so the
			// drag delta must flip to keep "content follows the finger".
			const int target = rev ? m_drag_start_scroll_h + dx : m_drag_start_scroll_h - dx;
			set_scroll_offset_horizontal(target);
			const int actual = m_scroll_offset_h - before;
			const int leftover = (target - before) - actual;
			if (leftover != 0) {
				forward_scroll_to_ancestor(leftover, true);
			}
			if (actual != 0) {
				dispatch_scrolled(actual, 0);
			}
		}
	} else {
		const float y = p_mm->get_position().y;
		m_velocity_tracker_v.add_sample(y, m_elapsed_ms);
		int dy = (int)y - m_drag_start_mouse;
		if (!m_drag_scrolled && (dy < -8 || dy > 8)) {
			m_drag_scrolled = true;
		}
		if (m_drag_scrolled) {
			const int before = m_scroll_offset;
			const int target = rev ? m_drag_start_scroll + dy : m_drag_start_scroll - dy;
			set_scroll_offset(target);
			const int actual = m_scroll_offset - before;
			const int leftover = (target - before) - actual;
			if (leftover != 0) {
				forward_scroll_to_ancestor(leftover, false);
			}
			if (actual != 0) {
				dispatch_scrolled(0, actual);
			}
		}
	}
}

void RecyclerView::end_drag() {
	finish_drag();
}

// Registers this RV as the drag grabber with every ancestor RV: each ancestor
// then routes unowned drag events to its registered grabber, so a gesture keeps
// tracking the RV even after the mouse leaves its bounds.
void RecyclerView::begin_drag_grabber_chain() {
	RecyclerView *ancestor = find_ancestor_recycler_view();
	if (ancestor != nullptr) {
		ancestor->set_drag_grabber(this);
		ancestor->begin_drag_grabber_chain();
	}
}

void RecyclerView::clear_drag_grabber_chain() {
	Node *node = get_parent();
	RecyclerView *prev = this;
	while (node != nullptr) {
		RecyclerView *rv = Object::cast_to<RecyclerView>(node);
		if (rv != nullptr && rv->m_drag_grabber == prev) {
			rv->m_drag_grabber = nullptr;
			prev = rv;
		}
		node = node->get_parent();
	}
}

RecyclerView *RecyclerView::find_ancestor_recycler_view() const {
	Node *node = get_parent();
	while (node != nullptr) {
		RecyclerView *rv = Object::cast_to<RecyclerView>(node);
		if (rv != nullptr) {
			return rv;
		}
		node = node->get_parent();
	}
	return nullptr;
}

RecyclerView *RecyclerView::find_ancestor_scrolling_axis(bool p_horizontal) const {
	Node *node = get_parent();
	while (node != nullptr) {
		RecyclerView *rv = Object::cast_to<RecyclerView>(node);
		if (rv != nullptr && rv->m_layout.is_valid()) {
			const bool can = p_horizontal ? rv->m_layout->can_scroll_horizontally() : rv->m_layout->can_scroll_vertically();
			if (can) {
				return rv;
			}
		}
		node = node->get_parent();
	}
	return nullptr;
}

bool RecyclerView::has_ancestor_scrolling_axis(bool p_horizontal) const {
	return find_ancestor_scrolling_axis(p_horizontal) != nullptr;
}

void RecyclerView::forward_scroll_to_ancestor(int p_delta, bool p_horizontal) {
	if (p_delta == 0) {
		return;
	}
	RecyclerView *ancestor = find_ancestor_recycler_view();
	if (ancestor != nullptr) {
		if (p_horizontal) {
			ancestor->forward_horizontal_scroll(p_delta);
		} else {
			ancestor->forward_vertical_scroll(p_delta);
		}
	}
}

// Routes a drag event to the RV that grabbed the gesture, converting its
// position from this RV's local space into the receiver's. Godot transforms
// event positions to the receiving control's local space, so without this the
// drag math would mix two coordinate systems and jump at the child boundary.
void RecyclerView::forward_event_to(RecyclerView *p_target, const Ref<InputEvent> &p_event) {
	if (p_target == nullptr) {
		return;
	}
	Ref<InputEventMouse> mouse = p_event;
	if (mouse.is_valid()) {
		mouse->set_position(mouse->get_position() + get_global_position() - p_target->get_global_position());
	}
	p_target->_gui_input(p_event);
}

// Finds the drag grabber: this RV's own, or the nearest ancestor RV's. This
// lets any RV (including a sibling nested RV like a chip row) route a live
// drag back to the descendant that owns it.
bool RecyclerView::forward_unowned_drag_event(const Ref<InputEvent> &p_event) {
	if (m_dragging) {
		return false;
	}
	RecyclerView *grabber = m_drag_grabber;
	if (grabber == nullptr) {
		Node *node = get_parent();
		while (node != nullptr && grabber == nullptr) {
			RecyclerView *rv = Object::cast_to<RecyclerView>(node);
			if (rv != nullptr && rv->m_drag_grabber != nullptr && !rv->m_dragging) {
				grabber = rv->m_drag_grabber;
			}
			node = node->get_parent();
		}
	}
	if (grabber != nullptr && grabber != this) {
		forward_event_to(grabber, p_event);
		return true;
	}
	return false;
}

// Cascades a vertical scroll gesture: consume on this RV's own vertical axis,
// forward the unconsumed remainder to the nearest ancestor RV. A horizontal RV
// only maps the vertical wheel to its axis when no ancestor scrolls vertically
// (the standalone vertical_wheel_scrolls_horizontal fallback is preserved).
void RecyclerView::forward_vertical_scroll(int p_delta) {
	if (p_delta == 0) {
		return;
	}
	if (m_layout.is_valid() && m_layout->can_scroll_vertically()) {
		const int consumed = scroll_vertically(p_delta);
		const int leftover = p_delta - consumed;
		if (leftover != 0) {
			forward_scroll_to_ancestor(leftover, false);
		}
		return;
	}
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()
			&& m_vertical_wheel_scrolls_horizontal && !has_ancestor_scrolling_axis(false)) {
		scroll_along_axis(p_delta);
		return;
	}
	forward_scroll_to_ancestor(p_delta, false);
}

void RecyclerView::forward_horizontal_scroll(int p_delta) {
	if (p_delta == 0) {
		return;
	}
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const int consumed = scroll_horizontally(p_delta);
		const int leftover = p_delta - consumed;
		if (leftover != 0) {
			forward_scroll_to_ancestor(leftover, true);
		}
		return;
	}
	forward_scroll_to_ancestor(p_delta, true);
}

void RecyclerView::add_on_scroll_listener(const Ref<ScrollListener> &p_listener) {
	if (p_listener.is_valid() && !m_scroll_listeners.has(p_listener)) {
		m_scroll_listeners.push_back(p_listener);
	}
}

void RecyclerView::remove_on_scroll_listener(const Ref<ScrollListener> &p_listener) {
	m_scroll_listeners.erase(p_listener);
}

void RecyclerView::clear_on_scroll_listeners() {
	m_scroll_listeners.clear();
}

void RecyclerView::set_item_animator(const Ref<ItemAnimator> &p_animator) {
	m_item_animator = p_animator;
	if (m_item_animator.is_valid()) {
		m_item_animator->set_recycler_view(this);
	}
}

void RecyclerView::set_item_touch_helper(const Ref<ItemTouchHelper> &p_helper) {
	m_item_touch_helper = p_helper;
}

void RecyclerView::set_snap_helper(const Ref<SnapHelper> &p_helper) {
	m_snap_helper = p_helper;
}

void RecyclerView::set_scroll_bar(RecyclerViewScrollBar *p_bar) {
	if (m_scroll_bar == p_bar) {
		return;
	}
	if (m_scroll_bar != nullptr) {
		m_scroll_bar->unbind();
		remove_child(m_scroll_bar);
	}
	m_scroll_bar = p_bar;
	if (m_scroll_bar != nullptr) {
		add_child(m_scroll_bar);
		m_scroll_bar->bind_to(this);
		m_scroll_bar->set_auto_hide(m_scroll_bar_auto_hide);
		m_scroll_bar->set_hide_delay(m_scroll_bar_hide_delay);
		m_scroll_bar->on_scroll_changed();
	}
}

void RecyclerView::set_scroll_bar_auto_hide(bool p_enabled) {
	m_scroll_bar_auto_hide = p_enabled;
	if (m_scroll_bar != nullptr) {
		m_scroll_bar->set_auto_hide(p_enabled);
	}
}

void RecyclerView::set_scroll_bar_hide_delay(float p_delay) {
	m_scroll_bar_hide_delay = p_delay;
	if (m_scroll_bar != nullptr) {
		m_scroll_bar->set_hide_delay(p_delay);
	}
}

void RecyclerView::smooth_scroll_to(int p_target, double p_duration) {
	stop_fling();
	const bool h = m_layout.is_valid() && m_layout->can_scroll_horizontally();
	m_settle_from = h ? m_scroll_offset_h : m_scroll_offset;
	m_settle_to = p_target;
	m_settle_elapsed = 0.0;
	m_settle_duration = p_duration;
	m_settle_active = true;
	set_scroll_state(SCROLL_STATE_SETTLING);
}

int RecyclerView::target_offset_for_position(int p_position) {
	if (m_layout.is_null()) {
		return 0;
	}
	// Scrolling to the item's content-space start aligns its leading edge to the
	// viewport start (normal layout) or its trailing edge to the viewport end
	// (reverse layout) — both are the same target. Clamped by set_scroll_offset.
	return m_layout->get_position_offset(p_position);
}

void RecyclerView::scroll_to_position(int p_position) {
	if (m_auto_measure_items) {
		// The offset table may still hold estimates for the target's region;
		// remember the target so the layout re-anchors once the measured
		// extents settle (port of Android's mPendingScrollPosition).
		m_pending_scroll_target = p_position;
		m_last_correct_raw = -1;
	}
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		set_scroll_offset_horizontal(target_offset_for_position(p_position));
	} else {
		set_scroll_offset(target_offset_for_position(p_position));
	}
}

void RecyclerView::smooth_scroll_to_position(int p_position, double p_duration) {
	if (m_auto_measure_items) {
		// Same pending target as scroll_to_position; corrected once the settle
		// animation finishes (see advance_settle), never mid-flight.
		m_pending_scroll_target = p_position;
		m_last_correct_raw = -1;
	}
	smooth_scroll_to(target_offset_for_position(p_position), p_duration);
}

void RecyclerView::advance_settle(double p_delta) {
	const bool h = m_layout.is_valid() && m_layout->can_scroll_horizontally();
	m_settle_elapsed += p_delta;
	const double t = m_settle_duration > 0.0 ? m_settle_elapsed / m_settle_duration : 1.0;
	int target;
	if (t >= 1.0) {
		target = m_settle_to;
		m_settle_active = false;
	} else {
		// Decelerate ease (matches Android's smooth-scroll-to-position feel).
		const double e = 1.0 - (1.0 - t) * (1.0 - t);
		target = (int)(m_settle_from + (m_settle_to - m_settle_from) * e);
	}
	const int before = h ? m_scroll_offset_h : m_scroll_offset;
	if (h) {
		set_scroll_offset_horizontal(target);
	} else {
		set_scroll_offset(target);
	}
	const int actual = (h ? m_scroll_offset_h : m_scroll_offset) - before;
	if (actual != 0) {
		dispatch_scrolled(h ? actual : 0, h ? 0 : actual);
	}
	if (!m_settle_active) {
		set_scroll_state(SCROLL_STATE_IDLE);
		// Auto-measure: the animated target was computed from estimates; once
		// the measured extents settle, re-anchor to the exact target (the
		// pending target set by smooth_scroll_to_position). This runs outside
		// any layout pass, so set_scroll_offset inside corrects and re-lays out.
		correct_pending_scroll_target();
	}
}

Ref<ViewHolder> RecyclerView::find_child_holder_at(const Vector2 &p_local_pos) {
	// Topmost first (children are in tree order).
	for (int i = m_children.size() - 1; i >= 0; i--) {
		const Ref<ViewHolder> &holder = m_children[i];
		Control *control = holder->get_control();
		if (control == nullptr) {
			continue;
		}
		// Hit-test against the LAYOUT slot (get_layout_position), not the control's
		// live position: during an ItemAnimator move the control is mid-flight
		// between slots, and a press on its slot would otherwise miss it (the
		// touch helper then never starts a swipe/drag on it).
		const Vector2 slot = get_layout_position(holder);
		if (Rect2(slot, control->get_size()).has_point(p_local_pos)) {
			return holder;
		}
	}
	return Ref<ViewHolder>();
}

bool RecyclerView::is_item_touch_occupied(const Ref<ViewHolder> &p_holder) const {
	return m_item_touch_helper.is_valid() && m_item_touch_helper->is_occupied(p_holder);
}

bool RecyclerView::on_failed_to_recycle_view(const Ref<ViewHolder> &p_holder) {
	if (m_adapter.is_valid()) {
		return m_adapter->on_failed_to_recycle_view(p_holder);
	}
	return false;
}

Vector2 RecyclerView::get_layout_position(const Ref<ViewHolder> &p_holder) {
	if (m_layout.is_null() || p_holder.is_null()) {
		return Vector2();
	}
	if (p_holder->get_position() < 0) {
		// A removed holder has NO_POSITION; resolve to its current position
		// instead of (0,0) so no lingering animation drags it to the origin.
		if (p_holder->get_control() != nullptr) {
			return p_holder->get_control()->get_position();
		}
		return Vector2();
	}
	const Rect2 rect = m_layout->get_item_rect(this, p_holder->get_position());
	const Vector4 insets = get_item_insets(p_holder->get_position());
	return rect.position + Vector2(insets.x, insets.y);
}

void RecyclerView::recycle_removed(const Ref<ViewHolder> &p_holder) {
	Control *control = p_holder->get_control();
	if (control != nullptr && control->get_parent() == this) {
		remove_child(control);
		// Port of Adapter.onViewDetachedFromWindow: the remove animation just
		// finished, so the control finally leaves the tree (it stayed attached
		// while fading out). It was already dropped from m_children, so
		// remove_item_view is not involved.
		if (m_adapter.is_valid()) {
			m_adapter->on_view_detached(p_holder);
		}
	}
	m_recycler->scrap_view(p_holder);
}

void RecyclerView::recycle_if_out_of_view(const Ref<ViewHolder> &p_holder) {
	if (m_layout.is_null() || p_holder.is_null()) {
		return;
	}
	if (!is_holder_out_of_view(p_holder)) {
		return;
	}
	// Port of Adapter.onFailedToRecycleView: a holder declared non-recyclable
	// (set_is_recyclable(false)) stays attached unless the adapter forces the
	// recycle; the decision is re-visited on later passes.
	if (!p_holder->is_recyclable() && !on_failed_to_recycle_view(p_holder)) {
		return;
	}
	// The holder just finished animating and its slot is fully outside the
	// viewport: return it to the pool immediately so rapid scrolling reuses it
	// instead of fabricating a fresh view each animation period.
	for (int i = 0; i < m_children.size(); i++) {
		if (m_children[i] == p_holder) {
			remove_item_view(p_holder);
			recycle_view(p_holder, p_holder->get_position());
			return;
		}
	}
}

void RecyclerView::capture_pre_positions() {
	m_pre_positions.clear();
	m_updated_holders.clear();
	for (int i = 0; i < m_children.size(); i++) {
		Ref<ViewHolder> holder = m_children[i];
		Vector2 position;
		if (holder->get_control() != nullptr) {
			position = holder->get_control()->get_position();
		}
		m_pre_positions.push_back({ holder, position });
	}
}

bool RecyclerView::in_pre_positions(const Ref<ViewHolder> &p_holder) const {
	for (int i = 0; i < m_pre_positions.size(); i++) {
		if (m_pre_positions[i].holder == p_holder) {
			return true;
		}
	}
	return false;
}

bool RecyclerView::is_holder_out_of_view(const Ref<ViewHolder> &p_holder) const {
	if (m_layout.is_null() || p_holder.is_null()) {
		return false;
	}
	const int pos = p_holder->get_position();
	if (pos < 0) {
		return false;  // Removed holders are handled by the remove animation.
	}
	const Rect2 rect = m_layout->get_item_rect(const_cast<RecyclerView *>(this), pos);
	const Vector2 viewport = get_viewport_size();
	if (m_layout->can_scroll_vertically()) {
		return rect.position.y + rect.size.y <= 0.0f || rect.position.y >= viewport.y;
	}
	return rect.position.x + rect.size.x <= 0.0f || rect.position.x >= viewport.x;
}

void RecyclerView::dispatch_animations() {
	for (int i = 0; i < m_pre_positions.size(); i++) {
		PrePosition &pre = m_pre_positions.write[i];
		if (is_item_touch_occupied(pre.holder)) {
			// The touch helper drives this holder's position itself (drag/swipe/
			// settle); an ItemAnimator move would fight it.
			continue;
		}
		if (m_removed_holders.has(pre.holder)) {
			// Data removed this cycle: fade the kept control out, recycle after.
			m_item_animator->animate_remove(pre.holder, Rect2(pre.position, Vector2()), Rect2());
		} else if (m_children.has(pre.holder)) {
			// Persisted: slide if it moved, pulse if it was rebound. Holders
			// merely recycled out of the visible range are skipped entirely.
			if (is_holder_out_of_view(pre.holder)) {
				// An item pushed out of the viewport by the update must be
				// recycled, not animated: its move would re-trigger on every
				// subsequent update (never finishing) and block recycling.
				continue;
			}
			Vector2 now = pre.position;
			if (pre.holder->get_control() != nullptr) {
				now = pre.holder->get_control()->get_position();
			}
			if (m_updated_holders.has(pre.holder)) {
				m_item_animator->animate_change(pre.holder, Rect2(pre.position, Vector2()), Rect2(now, Vector2()));
			} else if (now != pre.position) {
				m_item_animator->animate_move(pre.holder, Rect2(pre.position, Vector2()), Rect2(now, Vector2()));
			}
		}
	}
	// New holders this cycle: fade in.
	for (int i = 0; i < m_children.size(); i++) {
		Ref<ViewHolder> holder = m_children[i];
		if (is_item_touch_occupied(holder)) {
			continue;
		}
		if (!in_pre_positions(holder)) {
			Vector2 position;
			if (holder->get_control() != nullptr) {
				position = holder->get_control()->get_position();
			}
			m_item_animator->animate_add(holder, Rect2(), Rect2(position, Vector2()));
		}
	}
	m_pre_positions.clear();
}

void RecyclerView::set_adapter(const Ref<Adapter> &p_adapter) {
	detach_from_adapter();
	// A new adapter owns different data and creates different views: the
	// attached holders belong to the old adapter and would keep showing its
	// content (the fill loop cannot re-create them — their positions are
	// taken). Recycle the attached holders and drop every cached/pooled view
	// so the new adapter starts from an empty Recycler (Android's setAdapter
	// removes all views and clears the Recycler). Any auto-measured extents
	// are invalidated by mark_data_changed below.
	if (m_item_animator.is_valid()) {
		m_item_animator->clear();
	}
	for (int i = m_children.size() - 1; i >= 0; i--) {
		const Ref<ViewHolder> holder = m_children[i];
		remove_item_view(holder);
		m_recycler->recycle_view(holder, holder->get_position());
	}
	m_recycler->free_all_views();
	// The measured extents belong to the old adapter's content and views;
	// drop them (mark_data_changed no longer clears the cache wholesale).
	clear_measured_extents();
	m_extent_estimates.clear();
	m_adapter = p_adapter;
	attach_to_adapter();
	mark_data_changed();
	layout_children();
}

Ref<Adapter> RecyclerView::get_adapter() const {
	return m_adapter;
}

void RecyclerView::set_layout(const Ref<LayoutManager> &p_layout) {
	m_layout = p_layout;
	layout_children();
}

Ref<LayoutManager> RecyclerView::get_layout() const {
	return m_layout;
}

void RecyclerView::notify_item_range_changed(int p_position, int p_count, const Variant &p_payload) {
	m_adapter_helper->on_item_range_changed(p_position, p_count, p_payload);
	mark_data_changed();
	defer_layout();
}

void RecyclerView::notify_item_range_inserted(int p_position, int p_count) {
	m_adapter_helper->on_item_range_inserted(p_position, p_count);
	mark_data_changed();
	defer_layout();
}

void RecyclerView::notify_item_range_removed(int p_position, int p_count) {
	m_adapter_helper->on_item_range_removed(p_position, p_count);
	mark_data_changed();
	defer_layout();
}

void RecyclerView::notify_item_moved(int p_from_position, int p_to_position) {
	m_adapter_helper->on_item_range_moved(p_from_position, p_to_position);
	mark_data_changed();
	defer_layout();
}

void RecyclerView::notify_data_changed() {
	m_adapter_helper->clear();
	// A full data-set change makes every position unknown: drop all measured
	// extents (incremental notify_* ops shift the array instead, see
	// process_pending_updates).
	clear_measured_extents();
	mark_data_changed();
	defer_layout();
}

void RecyclerView::mark_data_changed() {
	if (m_layout.is_valid()) {
		m_layout->on_data_changed();
	}
}

void RecyclerView::clear_measured_extents() {
	if (!m_measured_extents.is_empty()) {
		m_measured_extents.fill(0);
	}
	if (!m_measured_expand_flags.is_empty()) {
		m_measured_expand_flags.fill(false);
	}
}

void RecyclerView::offset_measured_extents_for_ops(const Vector<UpdateOp> &p_ops) {
	if (m_measured_extents.is_empty()) {
		return;
	}
	for (int i = 0; i < p_ops.size(); i++) {
		const UpdateOp &op = p_ops[i];
		switch (op.cmd) {
			case UpdateOp::ADD: {
				for (int k = 0; k < op.item_count; k++) {
					m_measured_extents.insert(op.position_start, 0);
					m_measured_expand_flags.insert(op.position_start, false);
				}
				break;
			}
			case UpdateOp::REMOVE: {
				for (int k = 0; k < op.item_count; k++) {
					m_measured_extents.remove_at(op.position_start);
					m_measured_expand_flags.remove_at(op.position_start);
				}
				break;
			}
			case UpdateOp::MOVE: {
				const int value = m_measured_extents[op.position_start];
				const bool expand = m_measured_expand_flags[op.position_start];
				m_measured_extents.remove_at(op.position_start);
				m_measured_expand_flags.remove_at(op.position_start);
				m_measured_extents.insert(op.item_count, value);
				m_measured_expand_flags.insert(op.item_count, expand);
				break;
			}
			case UpdateOp::UPDATE: {
				for (int k = op.position_start; k < op.position_start + op.item_count; k++) {
					if (k >= 0 && k < m_measured_extents.size()) {
						m_measured_extents.write[k] = 0;
						m_measured_expand_flags.write[k] = false;
					}
				}
				// The changed rows' content (and thus height profile) may be
				// different: drop the historical estimates so they rebuild
				// from the re-measured rows.
				m_extent_estimates.clear();
				break;
			}
			default:
				break;
		}
	}
}

void RecyclerView::set_item_extent(int p_size) {
	m_item_extent = p_size;
	mark_data_changed();
}

void RecyclerView::set_auto_measure_items(bool p_enabled) {
	if (m_auto_measure_items == p_enabled) {
		return;
	}
	m_auto_measure_items = p_enabled;
	clear_measured_extents();
	m_extent_estimates.clear();
	mark_data_changed();
	defer_layout();
}

void RecyclerView::detach_from_adapter() {
	if (m_adapter.is_valid()) {
		m_adapter->unregister_adapter_data_observer(m_data_observer);
	}
}

void RecyclerView::attach_to_adapter() {
	if (m_adapter.is_valid()) {
		m_adapter->register_adapter_data_observer(m_data_observer);
		m_recycler->set_adapter(m_adapter);
	}
}

Ref<ViewHolder> RecyclerView::get_view_for_position(int p_position) {
	return m_recycler->get_view_for_position(p_position);
}

void RecyclerView::recycle_view(const Ref<ViewHolder> &p_holder, int p_position) {
	// Never recycle a holder whose item animation is still running.
	if (m_item_animator.is_valid() && m_item_animator->is_animating(p_holder)) {
		return;
	}
	// During an animated update layout (m_pre_positions holds the pre-update
	// positions), a holder pushed out of the viewport must not re-enter the
	// cache: the same cycle's fill could re-attach it at another position (a
	// head insert takes the tail holder) and the dispatch would animate it
	// from its old off-screen slot to the new one — the tail item flying to
	// the top. Route it to the changed scrap instead (port of Android's
	// changed-scrap): the fill only reuses a scrap holder whose position
	// matches exactly, and unused scrap sinks to the pool after the layout
	// (flush_scrap_to_pool), where later cycles reuse it normally.
	if (m_item_animator.is_valid() && !m_pre_positions.is_empty()) {
		m_recycler->scrap_view(p_holder);
		return;
	}
	m_recycler->recycle_view(p_holder, p_position);
}

void RecyclerView::add_item_view(const Ref<ViewHolder> &p_holder) {
	Control *control = p_holder->get_control();
	if (control != nullptr && control->get_parent() != this) {
		// The item root passes events through so the RecyclerView can scroll;
		// nested Controls keep their own mouse filter (their choice to interact).
		control->set_mouse_filter(MOUSE_FILTER_PASS);
		// A holder reused after a remove fade-out has a faded alpha; reset it.
		control->set_modulate(Color(1, 1, 1, 1));
		add_child(control);
		// Port of Adapter.onBindViewHolder: a holder that was mounted before
		// (its scene ran the ready pass, so @onready references are populated
		// and survive detach) can bind right away; FLAG_BOUND is cleared by
		// reset_internal for pool/cache reuses. A first-time mount is different:
		// Godot runs the item scene's ready pass at the end of the frame, NOT
		// inside add_child, so the control's @onready references are still null
		// right after the mount — binding now would run _bind_item against an
		// unready scene (a scene item refreshing its labels hits null refs).
		// Defer the first bind to the control's ready signal.
		if (m_adapter.is_valid() && !p_holder->is_bound()) {
			// An off-tree RV (build-time layout, unit tests) never runs ready
			// signals, so a first mount there must bind synchronously like
			// before — deferring would leave the holder forever unbound.
			// In-tree first mounts defer to the ready signal instead.
			if (p_holder->has_mounted_once() || !control->is_inside_tree() || control->is_node_ready()) {
				m_adapter->bind_view_holder(p_holder, p_holder->get_position());
			} else {
				Callable bind = callable_mp(this, &RecyclerView::_on_item_ready).bind(p_holder);
				control->connect("ready", bind, Object::CONNECT_ONE_SHOT);
			}
		}
		p_holder->mark_mounted_once();
		// Port of Adapter.onViewAttachedToWindow: the item Control just entered
		// the RecyclerView's tree, i.e. it is about to be seen by the user.
		// Reuses from the cache/pool re-attach and fire this again, matching
		// Android's attach/detach-on-scroll behavior.
		if (m_adapter.is_valid()) {
			m_adapter->on_view_attached(p_holder);
		}
	}
	m_children.push_back(p_holder);
}

void RecyclerView::_on_item_ready(const Ref<ViewHolder> &p_holder) {
	if (p_holder.is_null() || p_holder->is_bound() || m_adapter.is_null()) {
		return;
	}
	m_adapter->bind_view_holder(p_holder, p_holder->get_position());
	// Auto-measure: the layout measured this row from empty content (the bind
	// could not happen before the ready pass). Drop its measurement and re-run
	// the layout so the slot follows the bound content.
	if (m_auto_measure_items && m_layout.is_valid()) {
		const int pos = p_holder->get_position();
		if (pos >= 0 && pos < (int)m_measured_extents.size()) {
			m_measured_extents.write[pos] = 0;
			m_measured_expand_flags.write[pos] = false;
		}
		m_layout->on_data_changed();
		request_layout();
	}
}

void RecyclerView::remove_item_view(const Ref<ViewHolder> &p_holder) {
	// Keep animating holders attached: removing them would orphan their control
	// (the recycler skip below can't cache them), and the dispatch would then
	// misread them as removed. They are recycled once their animation finishes.
	if (m_item_animator.is_valid() && m_item_animator->is_animating(p_holder)) {
		return;
	}
	for (int i = 0; i < m_children.size(); i++) {
		if (m_children[i] == p_holder) {
			m_children.remove_at(i);
			break;
		}
	}
	Control *control = p_holder->get_control();
	if (control != nullptr && control->get_parent() == this) {
		remove_child(control);
		// Port of Adapter.onViewDetachedFromWindow: the item Control left the
		// RecyclerView's tree (scrolled off, removed, or recycled). Not
		// permanent: a later re-attach fires on_view_attached again.
		if (m_adapter.is_valid()) {
			m_adapter->on_view_detached(p_holder);
		}
	}
}

void RecyclerView::set_item_view_position(const Ref<ViewHolder> &p_holder, const Vector2 &p_pos, const Vector2 &p_size) {
	Control *control = p_holder->get_control();
	if (control == nullptr) {
		return;
	}
	// Auto-measure: the slot extent is decided by the item's content instead of
	// the static item extent. Hooked here because every layout manager (C++ or
	// script) funnels its fill through this call — the width is already fixed,
	// and the control is inside the tree (correct theme/fonts, and the min/max
	// cache invalidation is a no-op off-tree). The result is cached by position
	// and kept while the holder scrolls away and back (same position, same
	// content); any data change clears the cache and the next layout re-measures.
	const int pos = p_holder->get_position();
	if (m_auto_measure_items && pos >= 0 && pos < (int)m_measured_extents.size() && m_measured_extents[pos] <= 0) {
		const bool horizontal = m_layout.is_valid() && m_layout->can_scroll_horizontally();
		const int before = get_item_extent(pos);
		const int measured = measure_item_extent(p_holder, pos, horizontal ? p_size.y : p_size.x);
		if (measured > 0) {
			// -1 (<= 0) means the measurement ran off-tree and is unreliable
			// (see measure_item_extent): keep the estimate, the enter-tree
			// layout re-measures for real.
			m_measured_extents.write[pos] = measured;
			if (measured != before) {
				// The offset table was built with the estimate; the measured
				// value differs, so invalidate the table and run the layout
				// once more.
				m_layout->on_data_changed();
				m_layout_requested_again = true;
			}
		}
	}
	// Inset the item by the decorations' accumulated offsets so dividers/spacing
	// show in the gaps. Layout managers work with the uninflated geometry. Clamp
	// the size to zero so an offset larger than the item's extent can't collapse
	// the control into a negative size; spacing wider than the item must instead
	// be added to the reported extent (item height + gap) and offset back here.
	const Vector4 insets = get_item_insets(pos);
	const Vector2 final_pos = p_pos + Vector2(insets.x, insets.y);
	const Vector2 final_size = Vector2(
			MAX(0.0f, p_size.x - (insets.x + insets.z)),
			MAX(0.0f, p_size.y - (insets.y + insets.w)));
	control->set_position(final_pos);
	control->set_size(final_size);
}

// Port of Android's wrap_content / match_parent measurement. The item's
// control is measured at the width the layout assigns it (text wrapping
// depends on width), with the combined min/max caches explicitly invalidated
// first — set_size only emits RESIZED, which does not invalidate them, and
// both invalidation calls require the control to be inside the tree (the
// measurement point above guarantees that):
//   - default (wrap_content): extent = content size along the scroll axis
//     (combined minimum size), clamped by the combined maximum size when the
//     item declares one (custom_maximum_size or a script _get_maximum_size);
//   - SIZE_EXPAND on the root control along the scroll axis (match_parent):
//     extent = the RecyclerView viewport, so the control spans it.
// Decoration insets live outside the control (set_item_view_position insets
// the slot), so they are added to the returned extent.
int RecyclerView::measure_item_extent(const Ref<ViewHolder> &p_holder, int p_position, float p_width) {
	Control *control = p_holder->get_control();
	if (control == nullptr) {
		return m_item_extent;
	}
	// Off-tree measurements are unreliable: the min/max cache invalidation is
	// a no-op until the control enters the tree, so the caches can hold values
	// shaped at a wrong width. Return -1 (not cached; the estimate stands) and
	// let the enter-tree layout re-measure under a real tree.
	if (!control->is_inside_tree()) {
		return -1;
	}
	const bool vertical = m_layout.is_null() || m_layout->can_scroll_vertically();
	const Vector4 insets = get_item_insets(p_position);
	const float avail = vertical ? p_width - insets.x - insets.z : p_width - insets.y - insets.w;
	// Fix the cross-axis size first so wrapping/shaping use the final width,
	// then propagate it through the item's subtree: a container's minimum size
	// sums its children's, and a child's minimum is width-sensitive (a
	// fit_content RichTextLabel shapes at its current width — 0 before the
	// container lays out, which would inflate the measurement to one line per
	// character). preset_item_cross_size also invalidates each control's
	// min-size cache, since update_minimum_size only walks upward and a stale
	// child cache would survive and feed the container's sum.
	preset_item_cross_size(control, avail, vertical);
	// The combined maximum size (custom_maximum_size / a script
	// _get_maximum_size override) only exists on newer engines (4.5+); query it
	// dynamically so the extension still runs on older ones without the limit.
	float limit = -1.0f;
	if (control->has_method("get_combined_maximum_size")) {
		control->call("update_maximum_size");
		const Vector2 max_size = (Vector2)control->call("get_combined_maximum_size");
		limit = vertical ? max_size.y : max_size.x;
	}
	const float content = vertical ? control->get_combined_minimum_size().y : control->get_combined_minimum_size().x;
	int h;
	const BitField<Control::SizeFlags> flags = vertical ? control->get_v_size_flags() : control->get_h_size_flags();
	const bool is_expand = flags.has_flag(Control::SIZE_EXPAND);
	if (is_expand) {
		// match_parent: the slot is the whole viewport; the control itself then
		// spans the viewport minus the decoration insets.
		h = (int)(vertical ? get_viewport_size().y : get_viewport_size().x);
	} else {
		// wrap_content: ceil so a fractional content height never clips.
		h = (int)Math::ceil(content);
		// Adaptive per-view-type estimate: the running mean of measured
		// wrap_content extents for this view type. Skipped for match_parent
		// rows, which would pollute it with the viewport size. Unmeasured
		// rows fall back to it, so a row entering the viewport shifts the
		// offset table as little as possible.
		const int type = p_holder->get_item_view_type();
		ExtentEstimate &est = m_extent_estimates[type];
		est.sum += h;
		est.count++;
		est.value = est.sum / est.count;
	}
	// Record the expand flag (match_parent rows are viewport-derived and must
	// re-measure on a pure height change; wrap_content rows keep their values).
	if (p_position >= 0 && p_position < m_measured_expand_flags.size()) {
		m_measured_expand_flags.write[p_position] = is_expand;
	}
	// Remember the view type's expand-ness for the unmeasured estimate.
	{
		ExtentEstimate &est = m_extent_estimates[p_holder->get_item_view_type()];
		est.is_expand = is_expand;
	}
	if (limit >= 0.0f && h > (int)limit) {
		h = (int)limit;
	}
	h += (int)(vertical ? (insets.y + insets.w) : (insets.x + insets.z));
	return MAX(h, 0);
}

void RecyclerView::preset_item_cross_size(Control *p_control, float p_cross, bool p_vertical) {
	const Size2 current = p_control->get_size();
	p_control->set_size(p_vertical ? Vector2(p_cross, current.y) : Vector2(current.x, p_cross));
	// update_minimum_size invalidates upward only; a child cache that stays
	// valid would be returned stale by the parent's minimum-size sum. It is a
	// no-op off-tree, so this pass must run inside the tree (it does: the
	// measurement hooks into set_item_view_position).
	p_control->update_minimum_size();
	for (int i = 0; i < p_control->get_child_count(); i++) {
		Control *child = Object::cast_to<Control>(p_control->get_child(i));
		if (child != nullptr && !child->is_set_as_top_level()) {
			preset_item_cross_size(child, p_cross, p_vertical);
		}
	}
}

bool RecyclerView::correct_pending_scroll_target() {
	if (m_pending_scroll_target < 0 || m_settle_active || !m_layout.is_valid()) {
		return false;
	}
	const bool h = m_layout->can_scroll_horizontally();
	// The target's leading edge can lie past the maximum scroll offset (the
	// last item is shorter than the viewport, so its start is beyond
	// content - viewport). Clamp the way set_scroll_offset does, or the
	// re-anchor would never settle: it would re-apply a clamped offset every
	// layout pass and pin the list at the end, swallowing user drags.
	const int max = h ? get_max_scroll_offset_horizontal() : get_max_scroll_offset();
	const int raw = target_offset_for_position(m_pending_scroll_target);
	const int target = MIN(raw, max);
	const int current = h ? m_scroll_offset_h : m_scroll_offset;
	// Reached only when the target is in place and stable across two passes:
	// measured extents refine while rows enter the viewport, so a single-pass
	// match can be against a table that changes a moment later (the estimate
	// components move as rows measure, and the list would jump again).
	if (target == current && m_last_correct_raw == raw) {
		m_pending_scroll_target = -1;
		m_last_correct_raw = -1;
		return false;
	}
	m_last_correct_raw = raw;
	if (target != current) {
		if (h) {
			set_scroll_offset_horizontal(target);
		} else {
			set_scroll_offset(target);
		}
		return true;
	}
	// In place but the table is still refining: let the measurement-driven
	// re-runs call back in; do not force another pass ourselves.
	return false;
}

void RecyclerView::add_item_decoration(const Ref<ItemDecoration> &p_decor) {
	if (p_decor.is_valid()) {
		m_decorations.push_back(p_decor);
	}
	// Measured extents include the decoration insets; drop them all so the
	// next layout re-measures with the new insets.
	clear_measured_extents();
	mark_data_changed();
	queue_redraw();
}

void RecyclerView::remove_item_decoration(const Ref<ItemDecoration> &p_decor) {
	m_decorations.erase(p_decor);
	clear_measured_extents();
	mark_data_changed();
	queue_redraw();
}

Vector4 RecyclerView::get_item_insets(int p_position) const {
	Vector4 total;
	for (int i = 0; i < m_decorations.size(); i++) {
		const Vector4 insets = m_decorations[i]->get_item_offsets(p_position, const_cast<RecyclerView *>(this));
		total.x += insets.x;
		total.y += insets.y;
		total.z += insets.z;
		total.w += insets.w;
	}
	return total;
}

Rect2 RecyclerView::get_decorated_item_rect(int p_position) const {
	if (!m_layout.is_valid()) {
		return Rect2();
	}
	Rect2 rect = m_layout->get_item_rect(const_cast<RecyclerView *>(this), p_position);
	const Vector4 insets = get_item_insets(p_position);
	rect.position += Vector2(insets.x, insets.y);
	rect.size -= Vector2(insets.x + insets.z, insets.y + insets.w);
	return rect;
}

void RecyclerView::_draw() {
	for (int i = 0; i < m_decorations.size(); i++) {
		m_decorations[i]->on_draw(this);
	}
}

int RecyclerView::get_item_extent(int p_position) const {
	if (m_auto_measure_items && p_position >= 0 && p_position < m_measured_extents.size()) {
		const int measured = m_measured_extents[p_position];
		if (measured > 0) {
			return measured;
		}
	}
	if (m_adapter.is_valid()) {
		const int extent = m_adapter->get_item_extent(p_position);
		if (extent > 0) {
			return extent;
		}
	}
	// Unmeasured row with auto-measure: use the adaptive estimate for its
	// view type (smoothed recent measured extents) so the offset table
	// changes as little as possible when the row is measured on entry; fall
	// back to the static item extent (the Android-style default estimate).
	if (m_auto_measure_items && m_adapter.is_valid()) {
		const auto it = m_extent_estimates.find(m_adapter->get_item_view_type(p_position));
		if (it != m_extent_estimates.end()) {
			if (it->second.is_expand) {
				// An unmeasured match_parent row: its extent is the live
				// viewport size. Deriving it from the viewport (instead of a
				// stored mean) keeps the offset table right across resizes,
				// so the row entering the viewport measures exactly what the
				// table already says and nothing jumps.
				const bool vertical = m_layout.is_null() || m_layout->can_scroll_vertically();
				return (int)(vertical ? get_viewport_size().y : get_viewport_size().x);
			}
			if (it->second.value > 0) {
				return it->second.value;
			}
		}
	}
	return m_item_extent;
}

Ref<ViewHolder> RecyclerView::get_child_holder_at(int p_index) const {
	if (p_index < 0 || p_index >= m_children.size()) {
		return Ref<ViewHolder>();
	}
	return m_children[p_index];
}

int RecyclerView::get_max_scroll_offset() {
	int max_offset = 0;
	if (m_layout.is_valid()) {
		max_offset = m_layout->get_content_size(this) - (int)get_viewport_size().y;
	}
	if (max_offset < 0) {
		max_offset = 0;
	}
	return max_offset;
}

int RecyclerView::get_max_scroll_offset_horizontal() {
	int max_offset = 0;
	if (m_layout.is_valid()) {
		max_offset = m_layout->get_content_size(this) - (int)get_viewport_size().x;
	}
	if (max_offset < 0) {
		max_offset = 0;
	}
	return max_offset;
}

void RecyclerView::set_scroll_offset(int p_offset) {
	const int before = m_scroll_offset;
	m_scroll_offset = CLAMP(p_offset, 0, get_max_scroll_offset());
	if (m_scroll_offset != before) {
		m_last_scroll_direction = m_scroll_offset > before ? 1 : -1;
	}
	// A jump past the viewport (e.g. dragging the scroll bar) replaces the whole
	// visible set each frame; let this layout reuse the position-cached holders
	// by type too. Small scrolls keep the cache position-exact.
	const int viewport_main = (int)get_viewport_size().y;
	// A jump past the viewport (or an active scroll-bar drag) replaces the whole
	// visible set each frame; let this layout reuse the position-cached holders
	// by type too. Small scrolls keep the cache position-exact.
	m_recycler->set_cache_fallback_enabled(m_recycler->is_drag_buffering() || ABS(m_scroll_offset - before) > viewport_main);
	layout_children();
}

void RecyclerView::set_scroll_offset_horizontal(int p_offset) {
	const int before = m_scroll_offset_h;
	m_scroll_offset_h = CLAMP(p_offset, 0, get_max_scroll_offset_horizontal());
	if (m_scroll_offset_h != before) {
		m_last_scroll_direction = m_scroll_offset_h > before ? 1 : -1;
	}
	const int viewport_main = (int)get_viewport_size().x;
	m_recycler->set_cache_fallback_enabled(m_recycler->is_drag_buffering() || ABS(m_scroll_offset_h - before) > viewport_main);
	layout_children();
}

int RecyclerView::scroll_vertically(int p_delta) {
	int scrolled = 0;
	if (m_layout.is_valid() && m_layout->can_scroll_vertically()) {
		const int before = m_scroll_offset;
		set_scroll_offset(m_scroll_offset + p_delta);
		scrolled = m_scroll_offset - before;
	}
	if (scrolled != 0) {
		dispatch_scrolled(0, scrolled);
	}
	return scrolled;
}

int RecyclerView::scroll_horizontally(int p_delta) {
	int scrolled = 0;
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const int before = m_scroll_offset_h;
		set_scroll_offset_horizontal(m_scroll_offset_h + p_delta);
		scrolled = m_scroll_offset_h - before;
	}
	if (scrolled != 0) {
		dispatch_scrolled(scrolled, 0);
	}
	return scrolled;
}

Vector2 RecyclerView::get_viewport_size() const {
	return get_size();
}

void RecyclerView::process_pending_updates() {
	if (!m_adapter_helper->has_pending_updates()) {
		return;
	}
	// Keep the cache's positions consistent, then transform the attached holders.
	m_recycler->offset_position_records_for_ops(m_adapter_helper->get_pending_ops());
	// Measured extents are position-keyed too: shift them with the ops so the
	// untouched rows keep their measured values (a data change must not reset
	// the whole list to estimates — that would shrink the estimated content
	// below the scroll offset and make the layout jump as rows re-measure).
	offset_measured_extents_for_ops(m_adapter_helper->get_pending_ops());
	m_adapter_helper->consume_updates_in_one_pass(m_children);

	// FLAG_UPDATE is set here (and cleared by the rebind below), so capture the
	// updated holders for the change animation before the rebind.
	if (m_item_animator.is_valid()) {
		m_updated_holders.clear();
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i]->is_updated()) {
				m_updated_holders.push_back(m_children[i]);
			}
		}
	}

	// Drop holders whose item was removed. With an animator the control stays in
	// the tree for the fade-out (recycled on animation completion); without one
	// the holder goes straight to the changed scrap for reuse this cycle.
	m_removed_holders.clear();
	for (int i = m_children.size() - 1; i >= 0; i--) {
		Ref<ViewHolder> holder = m_children[i];
		if (holder->is_removed()) {
			if (m_item_animator.is_valid()) {
				m_removed_holders.push_back(holder);
				// Keep the control in the tree for the fade-out.
				m_children.remove_at(i);
			} else {
				remove_item_view(holder);
				m_recycler->scrap_view(holder);
			}
		}
	}

	// Re-bind holders whose item content changed (FLAG_UPDATE). A change op that
	// carried a payload triggers a partial rebind (only the affected child
	// control); otherwise the whole item is re-bound. bind_view_holder also
	// clears the FLAG_UPDATE marker.
	for (int i = 0; i < m_children.size(); i++) {
		Ref<ViewHolder> holder = m_children[i];
		if (holder->is_updated()) {
			const Variant payload = m_adapter_helper->get_payload_at_position(holder->get_position());
			m_adapter->bind_view_holder_with_payload(holder, holder->get_position(), payload);
		}
	}
}

void RecyclerView::layout_children() {
	if (m_layout_in_progress) {
		// A layout request (typically a RESIZED notification for the size being
		// finalized) arrived while a layout is running; re-run once afterwards
		// so the size/state change is not silently dropped.
		m_layout_requested_again = true;
		return;
	}
	m_layout_deferred = false;
	if (!m_layout.is_valid() || !m_adapter.is_valid()) {
		return;
	}
	m_layout_in_progress = true;
	// Auto-measure keeps the extent cache sized to the item count (0 = not
	// measured; the array is cleared on data changes instead of shifted).
	// resize_zeroed, not resize: the latter leaves freshly grown elements
	// uninitialized for trivially-constructible types, and get_item_extent
	// would treat a garbage positive value as a measured extent.
	if (m_auto_measure_items) {
		const int item_count = m_adapter->get_item_count();
		if (m_measured_extents.size() != item_count) {
			m_measured_extents.resize_zeroed(item_count);
			m_measured_expand_flags.resize_zeroed(item_count);
		}
	}
	int pass = 0;
	do {
		m_layout_requested_again = false;
		// Two-phase layout for item animations: capture pre-update positions,
		// then dispatch move/add/remove/change after the post layout. Only
		// incremental notify_* calls animate; plain scrolls/resizes do not.
		const bool has_updates = m_adapter_helper->has_pending_updates();
		if (has_updates && m_item_animator.is_valid()) {
			capture_pre_positions();
		}
		process_pending_updates();
		m_state->set_item_count(m_adapter->get_item_count());
		m_layout->set_recycler_view(this);
		// Size the view cache to the visible set before recycling anything: a
		// big jump then recycles the whole viewport into the cache without
		// discarding holders, and the recycler's miss-overflow keeps the pool
		// fed for the fill loop (port of Recycler.mViewCacheMax).
		m_recycler->update_view_cache_size(m_children.size());
		m_layout->on_layout_children(this, m_state.ptr());
		if (has_updates && m_item_animator.is_valid()) {
			dispatch_animations();
		}
		m_recycler->flush_scrap_to_pool();
		for (int i = 0; i < m_children.size(); i++) {
			m_children[i]->clear_old_position();
		}
		queue_redraw();
		// Auto-measure: once the measured extents settle, re-anchor the scroll
		// offset to the pending scroll target (port of Android's
		// mPendingScrollPosition) and run one more pass for the new offset.
		// set_scroll_offset re-enters layout_children, which only sets the
		// re-run flag while a layout is running, so this converges here.
		if (correct_pending_scroll_target()) {
			m_layout_requested_again = true;
		}
		// Auto-measure can shrink the content size between layouts: notify_*
		// clears the measured cache, unmeasured rows fall back to the
		// estimate, and the estimated content can be shorter than the current
		// offset. Nothing clamps the offset in that case (set_scroll_offset
		// does, but it is not called), so the fill range comes out empty and
		// the whole list looks gone. Clamp once; the next pass re-fills
		// normally as the rows measure and the estimate recovers.
		if (m_auto_measure_items) {
			const bool h = m_layout->can_scroll_horizontally();
			const int max = h ? get_max_scroll_offset_horizontal() : get_max_scroll_offset();
			int &off = h ? m_scroll_offset_h : m_scroll_offset;
			if (off > max) {
				off = max;
				m_layout_requested_again = true;
			}
		}
		pass++;
	} while (m_layout_requested_again && pass < MAX_AUTO_MEASURE_PASSES);
	m_layout_in_progress = false;
	// A pass cap may leave the re-run flag set; clear it so a later layout
	// starts fresh.
	m_layout_requested_again = false;
	m_recycler->set_cache_fallback_enabled(false);
	prefetch_adjacent();
	if (m_item_touch_helper.is_valid()) {
		// A swap relayout moved the dragged holder to its new slot; re-pin it to
		// the finger so it never tears visually (see ItemTouchHelper).
		m_item_touch_helper->on_after_layout(this);
	}
	if (m_scroll_bar != nullptr) {
		// Refresh the thumb after every scroll/layout (the bar reads the current
		// offset/content through the RecyclerViewScrollBar data contract).
		m_scroll_bar->on_scroll_changed();
	}
}

void RecyclerView::set_prefetch_enabled(bool p_enabled) {
	m_prefetch_enabled = p_enabled;
}

void RecyclerView::prefetch_adjacent() {
	if (m_layout.is_null() || m_adapter.is_null() || m_recycler.is_null()) {
		return;
	}
	// With auto-measure, the pre-measurement of the oncoming rows is a
	// correctness need, not an optimization: without it the rows measure on
	// entry and move every visible row (scrolling the other way jitters). So
	// the prefetch toggle gates the plain (reuse-only) prefetch; auto-measure
	// always pre-measures.
	if (!m_prefetch_enabled && !m_auto_measure_items) {
		return;
	}
	if (m_last_scroll_direction == 0) {
		return;
	}
	Array positions;
	m_layout->collect_adjacent_prefetch_positions(m_last_scroll_direction, this, positions);
	for (int i = 0; i < positions.size(); i++) {
		const int pos = (int)positions[i];
		if (m_auto_measure_items) {
			prefetch_and_measure(pos);
		} else {
			m_recycler->prefetch_view(pos);
		}
	}
}

void RecyclerView::prefetch_and_measure(int p_position) {
	Ref<ViewHolder> holder = m_recycler->get_view_for_position(p_position);
	if (holder.is_null()) {
		return;
	}
	Control *control = holder->get_control();
	// The measurement must run inside the tree: update_minimum_size is a
	// no-op off-tree, and a recycled holder's min-size cache would be stale
	// for its freshly bound content. The temporary attach has no other
	// side effect — the RV's adapter callbacks live in add/remove_item_view,
	// not in the bare add_child/remove_child used here.
	if (control != nullptr) {
		add_child(control);
		// The measurement reads the item content, so the holder must be bound
		// first. A holder that was mounted before can bind right away (its
		// @onready refs survive detach); a fresh holder cannot — its scene has
		// not run the ready pass yet (that happens at the end of the frame), so
		// binding would read null refs and the measurement would read empty
		// content. Skip the measurement; the first real mount binds and
		// measures this row instead.
		if (m_adapter.is_valid() && !holder->is_bound()) {
			// Off-tree RVs never run ready signals: bind synchronously. Only an
			// in-tree fresh mount must wait for its ready pass.
			if (!holder->has_mounted_once() && control->is_inside_tree()) {
				remove_child(control);
				m_recycler->recycle_view(holder, p_position);
				return;
			}
			m_adapter->bind_view_holder(holder, p_position);
		}
		const bool h = m_layout.is_valid() && m_layout->can_scroll_horizontally();
		const float width = h ? get_viewport_size().y : get_viewport_size().x;
		const int measured = measure_item_extent(holder, p_position, width);
		if (measured > 0) {
			m_measured_extents.write[p_position] = measured;
			// The offset table rebuilds on the next layout (the next scroll
			// pass) with the refined extents; the current display is not
			// affected because these rows are outside the viewport.
			m_layout->on_data_changed();
		}
		remove_child(control);
	}
	m_recycler->recycle_view(holder, p_position);
}

void RecyclerView::request_layout() {
	layout_children();
}

void RecyclerView::defer_layout() {
	if (m_layout_deferred) {
		return;
	}
	m_layout_deferred = true;
	call_deferred("layout_children");
}

void RecyclerView::free_items() {
	if (m_item_animator.is_valid()) {
		m_item_animator->clear();
	}
	for (int i = m_children.size() - 1; i >= 0; i--) {
		Control *control = m_children[i]->get_control();
		m_children.remove_at(i);
		if (control != nullptr) {
			if (control->get_parent() == this) {
				remove_child(control);
			}
			memdelete(control);
		}
	}
	m_recycler->free_all_views();
}

} // namespace godot
