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
