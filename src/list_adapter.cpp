#include "list_adapter.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void ListAdapter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_diff_callback", "callback"), &ListAdapter::set_diff_callback);
	ClassDB::bind_method(D_METHOD("get_diff_callback"), &ListAdapter::get_diff_callback);
	ClassDB::bind_method(D_METHOD("submit_list", "list"), &ListAdapter::submit_list);
	ClassDB::bind_method(D_METHOD("get_current_list"), &ListAdapter::get_current_list);
	ClassDB::bind_method(D_METHOD("get_item", "index"), &ListAdapter::get_item);
	GDVIRTUAL_BIND(_on_current_list_changed, "previous_list", "current_list");
}

ListAdapter::ListAdapter() {
	m_update_callback.instantiate();
	m_update_callback->set_adapter(this);
}

void ListAdapter::set_diff_callback(const Ref<DiffUtilItemCallback> &p_callback) {
	m_diff_callback = p_callback;
}

void ListAdapter::submit_list(const Array &p_list) {
	// Same Array instance: nothing to do (mirrors AsyncListDiffer.submitList).
	if (p_list == m_list) {
		return;
	}
	const Array previous = m_list;
	if (m_diff_callback.is_valid()) {
		Ref<ItemDiffCallback> cb;
		cb.instantiate();
		cb->item_callback = m_diff_callback;
		cb->old_items = previous;
		cb->new_items = p_list;
		// Commit the new list before dispatching so the adapter's item count
		// matches the updates the RecyclerView is about to receive.
		m_list = p_list;
		Ref<DiffResult> result = DiffUtil::calculate_diff(cb, true);
		result->dispatch_updates_to(m_update_callback);
	} else {
		// No comparator configured: fall back to a full replace.
		m_list = p_list;
		if (!previous.is_empty()) {
			m_update_callback->on_removed(0, previous.size());
		}
		if (!p_list.is_empty()) {
			m_update_callback->on_inserted(0, p_list.size());
		}
	}
	GDVIRTUAL_CALL(_on_current_list_changed, previous, m_list);
}

Variant ListAdapter::get_item(int p_index) const {
	ERR_FAIL_COND_V(p_index < 0 || p_index >= m_list.size(), Variant());
	return m_list[p_index];
}

int ListAdapter::get_item_count() {
	return m_list.size();
}

const void *ListAdapter::ItemDiffCallback::get_change_payload(int p_old_item_position, int p_new_item_position) {
	m_payload_buffer = item_callback->get_change_payload(old_items[p_old_item_position], new_items[p_new_item_position]);
	return &m_payload_buffer;
}

} // namespace godot
