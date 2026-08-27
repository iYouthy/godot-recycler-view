#include "recycler_view.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/error_macros.hpp>

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
	ClassDB::bind_method(D_METHOD("set_vertical_wheel_scrolls_horizontal", "enabled"), &RecyclerView::set_vertical_wheel_scrolls_horizontal);
	ClassDB::bind_method(D_METHOD("get_vertical_wheel_scrolls_horizontal"), &RecyclerView::get_vertical_wheel_scrolls_horizontal);
	ClassDB::bind_method(D_METHOD("set_item_size", "size"), &RecyclerView::set_item_size);
	ClassDB::bind_method(D_METHOD("get_item_size"), &RecyclerView::get_item_size);
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
	detach_from_adapter();
}

void RecyclerView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_RESIZED:
			layout_children();
			break;
	}
}

void RecyclerView::_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		if (mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				m_dragging = true;
				m_drag_start_mouse = (int)mb->get_position().y;
				m_drag_start_mouse_x = (int)mb->get_position().x;
				m_drag_start_scroll = m_scroll_offset;
				m_drag_start_scroll_h = m_scroll_offset_h;
				m_drag_scrolled = false;
			} else {
				m_dragging = false;
			}
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_UP) {
			scroll_along_axis(-(int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_DOWN) {
			scroll_along_axis((int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_LEFT) {
			scroll_horizontally(-(int)(mb->get_factor() * 48.0f));
			accept_event();
		} else if (mb->is_pressed() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_WHEEL_RIGHT) {
			scroll_horizontally((int)(mb->get_factor() * 48.0f));
			accept_event();
		}
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		// A release outside the window never reaches _gui_input, so the drag
		// must be re-validated against the current button mask on every motion.
		if (!mm->get_button_mask().has_flag(MouseButtonMask::MOUSE_BUTTON_MASK_LEFT)) {
			m_dragging = false;
			m_drag_scrolled = false;
			return;
		}
		if (m_dragging) {
			if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
				int dx = (int)mm->get_position().x - m_drag_start_mouse_x;
				if (!m_drag_scrolled && (dx < -8 || dx > 8)) {
					m_drag_scrolled = true;
				}
				if (m_drag_scrolled) {
					set_scroll_offset_horizontal(m_drag_start_scroll_h - dx);
				}
			} else {
				int dy = (int)mm->get_position().y - m_drag_start_mouse;
				if (!m_drag_scrolled && (dy < -8 || dy > 8)) {
					m_drag_scrolled = true;
				}
				if (m_drag_scrolled) {
					set_scroll_offset(m_drag_start_scroll - dy);
				}
			}
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
	request_layout();
}

void RecyclerView::notify_item_range_inserted(int p_position, int p_count) {
	m_adapter_helper->on_item_range_inserted(p_position, p_count);
	mark_data_changed();
	request_layout();
}

void RecyclerView::notify_item_range_removed(int p_position, int p_count) {
	m_adapter_helper->on_item_range_removed(p_position, p_count);
	mark_data_changed();
	request_layout();
}

void RecyclerView::notify_item_moved(int p_from_position, int p_to_position) {
	m_adapter_helper->on_item_range_moved(p_from_position, p_to_position);
	mark_data_changed();
	request_layout();
}

void RecyclerView::notify_data_changed() {
	m_adapter_helper->clear();
	mark_data_changed();
	request_layout();
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
	m_recycler->recycle_view(p_holder, p_position);
}

void RecyclerView::add_item_view(const Ref<ViewHolder> &p_holder) {
	Control *control = p_holder->get_control();
	if (control != nullptr && control->get_parent() != this) {
		// The item root passes events through so the RecyclerView can scroll;
		// nested Controls keep their own mouse filter (their choice to interact).
		control->set_mouse_filter(MOUSE_FILTER_PASS);
		add_child(control);
	}
	m_children.push_back(p_holder);
}

void RecyclerView::remove_item_view(const Ref<ViewHolder> &p_holder) {
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

void RecyclerView::set_scroll_offset(int p_offset) {
	int max_offset = 0;
	if (m_layout.is_valid()) {
		max_offset = m_layout->get_content_size(this) - (int)get_viewport_size().y;
	}
	if (max_offset < 0) {
		max_offset = 0;
	}
	m_scroll_offset = CLAMP(p_offset, 0, max_offset);
	layout_children();
}

void RecyclerView::set_scroll_offset_horizontal(int p_offset) {
	int max_offset = 0;
	if (m_layout.is_valid()) {
		max_offset = m_layout->get_content_size(this) - (int)get_viewport_size().x;
	}
	if (max_offset < 0) {
		max_offset = 0;
	}
	m_scroll_offset_h = CLAMP(p_offset, 0, max_offset);
	layout_children();
}

void RecyclerView::scroll_vertically(int p_delta) {
	if (m_layout.is_valid() && m_layout->can_scroll_vertically()) {
		set_scroll_offset(m_scroll_offset + p_delta);
	}
}

void RecyclerView::scroll_horizontally(int p_delta) {
	if (m_layout.is_valid() && m_layout->can_scroll_horizontally()) {
		set_scroll_offset_horizontal(m_scroll_offset_h + p_delta);
	}
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

	// Drop holders whose item was removed; keep them in the changed scrap so the
	// same view can be reused within this layout cycle.
	for (int i = m_children.size() - 1; i >= 0; i--) {
		Ref<ViewHolder> holder = m_children[i];
		if (holder->is_removed()) {
			remove_item_view(holder);
			m_recycler->scrap_view(holder);
		}
	}

	// Re-bind holders whose item content changed (FLAG_UPDATE). bind_view_holder
	// also clears the FLAG_UPDATE marker.
	for (int i = 0; i < m_children.size(); i++) {
		Ref<ViewHolder> holder = m_children[i];
		if (holder->is_updated()) {
			m_adapter->bind_view_holder(holder, holder->get_position());
		}
	}
}

void RecyclerView::layout_children() {
	if (m_layout_in_progress) {
		return;
	}
	if (!m_layout.is_valid() || !m_adapter.is_valid()) {
		return;
	}
	m_layout_in_progress = true;
	process_pending_updates();
	m_state->set_item_count(m_adapter->get_item_count());
	m_layout->set_recycler_view(this);
	m_layout->on_layout_children(this, m_state.ptr());
	m_recycler->flush_scrap_to_pool();
	for (int i = 0; i < m_children.size(); i++) {
		m_children[i]->clear_old_position();
	}
	queue_redraw();
	m_layout_in_progress = false;
}

void RecyclerView::request_layout() {
	layout_children();
}

void RecyclerView::free_items() {
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
