#include "layout_manager.h"

#include "recycler_view.h"
#include "state.h"

namespace godot {

void LayoutManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_recycler_view"), &LayoutManager::get_recycler_view);
	ClassDB::bind_method(D_METHOD("get_item_count"), &LayoutManager::get_item_count);
	ClassDB::bind_method(D_METHOD("get_content_size", "recycler_view"), &LayoutManager::get_content_size);
	ClassDB::bind_method(D_METHOD("get_position_offset", "position"), &LayoutManager::get_position_offset);
	ClassDB::bind_method(D_METHOD("set_reverse_layout", "reverse"), &LayoutManager::set_reverse_layout);
	ClassDB::bind_method(D_METHOD("get_reverse_layout"), &LayoutManager::get_reverse_layout);
	GDVIRTUAL_BIND(_on_layout_children, "recycler_view", "state");
	GDVIRTUAL_BIND(_can_scroll_vertically);
	GDVIRTUAL_BIND(_can_scroll_horizontally);
	GDVIRTUAL_BIND(_get_content_size, "recycler_view");
	GDVIRTUAL_BIND(_get_item_rect, "recycler_view", "position");
	GDVIRTUAL_BIND(_get_position_offset, "position");
	GDVIRTUAL_BIND(_on_data_changed);
	GDVIRTUAL_BIND(_collect_adjacent_prefetch_positions, "dy");
}

void LayoutManager::on_layout_children(RecyclerView *p_recycler_view, State *p_state) {
	// The GDVIRTUAL2_REQUIRED variant warns once when a script subclass does
	// not override _on_layout_children, then falls through to an empty layout.
	GDVIRTUAL_CALL(_on_layout_children, p_recycler_view, Ref<State>(p_state));
}

bool LayoutManager::can_scroll_vertically() const {
	bool result = false;
	GDVIRTUAL_CALL(_can_scroll_vertically, result);
	return result;
}

bool LayoutManager::can_scroll_horizontally() const {
	bool result = false;
	GDVIRTUAL_CALL(_can_scroll_horizontally, result);
	return result;
}

int LayoutManager::get_content_size(RecyclerView *p_recycler_view) const {
	int result = 0;
	GDVIRTUAL_CALL(_get_content_size, p_recycler_view, result);
	return result;
}

Rect2 LayoutManager::get_item_rect(RecyclerView *p_recycler_view, int p_position) const {
	Rect2 result;
	GDVIRTUAL_CALL(_get_item_rect, p_recycler_view, p_position, result);
	return result;
}

int LayoutManager::get_position_offset(int p_position) const {
	int result = 0;
	GDVIRTUAL_CALL(_get_position_offset, p_position, result);
	return result;
}

void LayoutManager::on_data_changed() {
	GDVIRTUAL_CALL(_on_data_changed);
}

void LayoutManager::collect_adjacent_prefetch_positions(int p_dy, RecyclerView *p_recycler_view, Array &r_positions) const {
	Array result;
	if (GDVIRTUAL_CALL(_collect_adjacent_prefetch_positions, p_dy, result)) {
		r_positions = result;
	}
}

void LayoutManager::set_recycler_view(RecyclerView *p_recycler_view) {
	m_recycler_view = p_recycler_view;
}

void LayoutManager::set_reverse_layout(bool p_reverse) {
	m_reverse_layout = p_reverse;
	if (m_recycler_view != nullptr) {
		m_recycler_view->request_layout();
	}
}

int LayoutManager::get_item_count() const {
	if (m_recycler_view == nullptr) {
		return 0;
	}
	Ref<Adapter> adapter = m_recycler_view->get_adapter();
	return adapter.is_valid() ? adapter->get_item_count() : 0;
}

} // namespace godot
