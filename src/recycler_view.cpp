#include "recycler_view.h"

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
	ClassDB::bind_method(D_METHOD("set_item_size", "size"), &RecyclerView::set_item_size);
	ClassDB::bind_method(D_METHOD("get_item_size"), &RecyclerView::get_item_size);
	ClassDB::bind_method(D_METHOD("set_prefetch_enabled", "enabled"), &RecyclerView::set_prefetch_enabled);
	ClassDB::bind_method(D_METHOD("get_prefetch_enabled"), &RecyclerView::get_prefetch_enabled);
	ClassDB::bind_method(D_METHOD("get_item_height", "position"), &RecyclerView::get_item_height);
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

	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_IDLE", SCROLL_STATE_IDLE);
	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_DRAGGING", SCROLL_STATE_DRAGGING);
	ClassDB::bind_integer_constant(get_class_static(), "ScrollState", "SCROLL_STATE_SETTLING", SCROLL_STATE_SETTLING);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "adapter", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_adapter", "get_adapter");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "layout", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_layout", "get_layout");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_size"), "set_item_size", "get_item_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vertical_wheel_scrolls_horizontal"), "set_vertical_wheel_scrolls_horizontal", "get_vertical_wheel_scrolls_horizontal");
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
			break;
		case NOTIFICATION_RESIZED:
			layout_children();
			break;
	}
}

void RecyclerView::_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
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
	if (m_item_animator.is_valid() && m_item_animator->is_running()) {
		m_item_animator->animate_step(p_delta);
	}
	if (m_scroll_state != SCROLL_STATE_SETTLING) {
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
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const int max_offset = get_max_scroll_offset_horizontal();
		m_fling_h.fling(m_scroll_offset_h, -p_velocity, 0, max_offset);
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
		m_fling_v.fling(m_scroll_offset, -p_velocity, 0, max_offset);
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
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		const float x = p_mm->get_position().x;
		m_velocity_tracker_h.add_sample(x, m_elapsed_ms);
		int dx = (int)x - m_drag_start_mouse_x;
		if (!m_drag_scrolled && (dx < -8 || dx > 8)) {
			m_drag_scrolled = true;
		}
		if (m_drag_scrolled) {
			const int before = m_scroll_offset_h;
			const int target = m_drag_start_scroll_h - dx;
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
			const int target = m_drag_start_scroll - dy;
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
	}
	m_recycler->scrap_view(p_holder);
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

void RecyclerView::dispatch_animations() {
	for (int i = 0; i < m_pre_positions.size(); i++) {
		PrePosition &pre = m_pre_positions.write[i];
		if (m_removed_holders.has(pre.holder)) {
			// Data removed this cycle: fade the kept control out, recycle after.
			m_item_animator->animate_remove(pre.holder, Rect2(pre.position, Vector2()), Rect2());
		} else if (m_children.has(pre.holder)) {
			// Persisted: slide if it moved, pulse if it was rebound. Holders
			// merely recycled out of the visible range are skipped entirely.
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
	mark_data_changed();
	defer_layout();
}

void RecyclerView::mark_data_changed() {
	if (m_layout.is_valid()) {
		m_layout->on_data_changed();
	}
}

void RecyclerView::set_item_size(int p_size) {
	m_item_size = p_size;
	mark_data_changed();
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
	}
	m_children.push_back(p_holder);
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
	}
}

void RecyclerView::set_item_view_position(const Ref<ViewHolder> &p_holder, const Vector2 &p_pos, const Vector2 &p_size) {
	Control *control = p_holder->get_control();
	if (control == nullptr) {
		return;
	}
	// Inset the item by the decorations' accumulated offsets so dividers/spacing
	// show in the gaps. Layout managers work with the uninflated geometry.
	const Vector4 insets = get_item_insets(p_holder->get_position());
	const Vector2 final_pos = p_pos + Vector2(insets.x, insets.y);
	const Vector2 final_size = p_size - Vector2(insets.x + insets.z, insets.y + insets.w);
	control->set_position(final_pos);
	control->set_size(final_size);
}

void RecyclerView::add_item_decoration(const Ref<ItemDecoration> &p_decor) {
	if (p_decor.is_valid()) {
		m_decorations.push_back(p_decor);
	}
	mark_data_changed();
	queue_redraw();
}

void RecyclerView::remove_item_decoration(const Ref<ItemDecoration> &p_decor) {
	m_decorations.erase(p_decor);
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

int RecyclerView::get_item_height(int p_position) const {
	if (m_adapter.is_valid()) {
		const int height = m_adapter->get_item_height(p_position);
		if (height > 0) {
			return height;
		}
	}
	return m_item_size;
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
	layout_children();
}

void RecyclerView::set_scroll_offset_horizontal(int p_offset) {
	const int before = m_scroll_offset_h;
	m_scroll_offset_h = CLAMP(p_offset, 0, get_max_scroll_offset_horizontal());
	if (m_scroll_offset_h != before) {
		m_last_scroll_direction = m_scroll_offset_h > before ? 1 : -1;
	}
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
		m_layout->on_layout_children(this, m_state.ptr());
		if (has_updates && m_item_animator.is_valid()) {
			dispatch_animations();
		}
		m_recycler->flush_scrap_to_pool();
		for (int i = 0; i < m_children.size(); i++) {
			m_children[i]->clear_old_position();
		}
		queue_redraw();
	} while (m_layout_requested_again);
	m_layout_in_progress = false;
	prefetch_adjacent();
}

void RecyclerView::set_prefetch_enabled(bool p_enabled) {
	m_prefetch_enabled = p_enabled;
}

void RecyclerView::prefetch_adjacent() {
	if (m_layout.is_null() || m_adapter.is_null() || m_recycler.is_null()) {
		return;
	}
	if (!m_prefetch_enabled) {
		return;
	}
	if (m_last_scroll_direction == 0) {
		return;
	}
	Array positions;
	m_layout->collect_adjacent_prefetch_positions(m_last_scroll_direction, this, positions);
	for (int i = 0; i < positions.size(); i++) {
		m_recycler->prefetch_view((int)positions[i]);
	}
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
