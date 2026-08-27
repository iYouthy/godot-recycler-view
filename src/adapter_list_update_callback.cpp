#include "adapter_list_update_callback.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void AdapterListUpdateCallback::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_adapter", "adapter"), &AdapterListUpdateCallback::set_adapter);
	ClassDB::bind_method(D_METHOD("get_adapter"), &AdapterListUpdateCallback::get_adapter);
}

void AdapterListUpdateCallback::set_adapter(const Ref<Adapter> &p_adapter) {
	m_adapter = p_adapter;
}

Ref<Adapter> AdapterListUpdateCallback::get_adapter() const {
	return m_adapter;
}

void AdapterListUpdateCallback::on_inserted(int p_position, int p_count) {
	if (m_adapter.is_valid()) {
		m_adapter->notify_item_range_inserted(p_position, p_count);
	}
}

void AdapterListUpdateCallback::on_removed(int p_position, int p_count) {
	if (m_adapter.is_valid()) {
		m_adapter->notify_item_range_removed(p_position, p_count);
	}
}

void AdapterListUpdateCallback::on_moved(int p_from_position, int p_to_position) {
	if (m_adapter.is_valid()) {
		m_adapter->notify_item_moved(p_from_position, p_to_position);
	}
}

void AdapterListUpdateCallback::on_changed(int p_position, int p_count, const Variant &p_payload) {
	if (m_adapter.is_valid()) {
		m_adapter->notify_item_range_changed(p_position, p_count, p_payload);
	}
}

} // namespace godot
