#include "adapter.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void AdapterDataObserver::_bind_methods() {
	GDVIRTUAL_BIND(_on_changed);
	GDVIRTUAL_BIND(_on_item_range_changed, "position", "count", "payload");
	GDVIRTUAL_BIND(_on_item_range_inserted, "position", "count");
	GDVIRTUAL_BIND(_on_item_range_removed, "position", "count");
	GDVIRTUAL_BIND(_on_item_moved, "from_position", "to_position");
	GDVIRTUAL_BIND(_on_state_restoration_policy_changed);
}

void AdapterDataObserver::on_changed() {
	GDVIRTUAL_CALL(_on_changed);
}

void AdapterDataObserver::on_item_range_changed(int p_position, int p_count, const Variant &p_payload) {
	GDVIRTUAL_CALL(_on_item_range_changed, p_position, p_count, p_payload);
}

void AdapterDataObserver::on_item_range_inserted(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_item_range_inserted, p_position, p_count);
}

void AdapterDataObserver::on_item_range_removed(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_item_range_removed, p_position, p_count);
}

void AdapterDataObserver::on_item_moved(int p_from_position, int p_to_position) {
	GDVIRTUAL_CALL(_on_item_moved, p_from_position, p_to_position);
}

void AdapterDataObserver::on_state_restoration_policy_changed() {
	GDVIRTUAL_CALL(_on_state_restoration_policy_changed);
}

// ---------------------------------------------------------------------------
// Adapter.

void Adapter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_view_holder", "parent", "view_type"), &Adapter::create_view_holder);
	ClassDB::bind_method(D_METHOD("bind_view_holder", "holder", "position"), &Adapter::bind_view_holder);
	ClassDB::bind_method(D_METHOD("bind_view_holder_with_payload", "holder", "position", "payload"), &Adapter::bind_view_holder_with_payload);
	ClassDB::bind_method(D_METHOD("get_item_count"), &Adapter::get_item_count);
	ClassDB::bind_method(D_METHOD("get_item_view_type", "position"), &Adapter::get_item_view_type);
	ClassDB::bind_method(D_METHOD("get_item_id", "position"), &Adapter::get_item_id);
	ClassDB::bind_method(D_METHOD("has_stable_ids"), &Adapter::has_stable_ids);
	ClassDB::bind_method(D_METHOD("set_has_stable_ids", "has_stable_ids"), &Adapter::set_has_stable_ids);
	ClassDB::bind_method(D_METHOD("has_observers"), &Adapter::has_observers);
	ClassDB::bind_method(D_METHOD("register_adapter_data_observer", "observer"), &Adapter::register_adapter_data_observer);
	ClassDB::bind_method(D_METHOD("unregister_adapter_data_observer", "observer"), &Adapter::unregister_adapter_data_observer);
	ClassDB::bind_method(D_METHOD("notify_data_set_changed"), &Adapter::notify_data_set_changed);
	ClassDB::bind_method(D_METHOD("notify_item_range_changed", "position", "count", "payload"), &Adapter::notify_item_range_changed);
	ClassDB::bind_method(D_METHOD("notify_item_range_inserted", "position", "count"), &Adapter::notify_item_range_inserted);
	ClassDB::bind_method(D_METHOD("notify_item_range_removed", "position", "count"), &Adapter::notify_item_range_removed);
	ClassDB::bind_method(D_METHOD("notify_item_moved", "from_position", "to_position"), &Adapter::notify_item_moved);
	ClassDB::bind_method(D_METHOD("notify_item_changed", "position"), &Adapter::notify_item_changed);
	ClassDB::bind_method(D_METHOD("notify_item_inserted", "position"), &Adapter::notify_item_inserted);
	ClassDB::bind_method(D_METHOD("notify_item_removed", "position"), &Adapter::notify_item_removed);
	GDVIRTUAL_BIND(_create_item, "parent", "view_type");
	GDVIRTUAL_BIND(_bind_item, "holder", "position");
	GDVIRTUAL_BIND(_bind_item_with_payload, "holder", "position", "payload");
	GDVIRTUAL_BIND(_get_item_count);
	GDVIRTUAL_BIND(_get_item_view_type, "position");
	GDVIRTUAL_BIND(_get_item_height, "position");
	GDVIRTUAL_BIND(_get_item_id, "position");
	GDVIRTUAL_BIND(_on_item_recycled, "holder");
	GDVIRTUAL_BIND(_on_failed_to_recycle_view, "holder");
	GDVIRTUAL_BIND(_on_view_attached_to_window, "holder");
	GDVIRTUAL_BIND(_on_view_detached_from_window, "holder");
}

Ref<ViewHolder> Adapter::create_view_holder(Control *p_parent, int p_view_type) {
	Ref<ViewHolder> holder;
	if (GDVIRTUAL_CALL(_create_item, p_parent, p_view_type, holder)) {
		if (holder.is_valid()) {
			holder->set_item_view_type(p_view_type);
			return holder;
		}
	}
	return Ref<ViewHolder>();
}

void Adapter::bind_view_holder(const Ref<ViewHolder> &p_holder, int p_position) {
	ERR_FAIL_NULL(p_holder);
	p_holder->set_position(p_position);
	if (has_stable_ids()) {
		p_holder->set_stable_id(get_item_id(p_position));
	}
	p_holder->set_flags(ViewHolder::FLAG_BOUND, ViewHolder::FLAG_BOUND | ViewHolder::FLAG_UPDATE | ViewHolder::FLAG_INVALID | ViewHolder::FLAG_ADAPTER_POSITION_UNKNOWN);
	GDVIRTUAL_CALL(_bind_item, p_holder, p_position);
}

void Adapter::bind_view_holder_with_payload(const Ref<ViewHolder> &p_holder, int p_position, const Variant &p_payload) {
	ERR_FAIL_NULL(p_holder);
	p_holder->set_position(p_position);
	if (has_stable_ids()) {
		p_holder->set_stable_id(get_item_id(p_position));
	}
	p_holder->set_flags(ViewHolder::FLAG_BOUND, ViewHolder::FLAG_BOUND | ViewHolder::FLAG_UPDATE | ViewHolder::FLAG_INVALID | ViewHolder::FLAG_ADAPTER_POSITION_UNKNOWN);
	// Partial rebind: a payload is set and the script implements the payload
	// hook, so only the affected child control is updated. Otherwise the whole
	// item is re-bound.
	if (p_payload.get_type() != Variant::NIL && GDVIRTUAL_CALL(_bind_item_with_payload, p_holder, p_position, p_payload)) {
		return;
	}
	GDVIRTUAL_CALL(_bind_item, p_holder, p_position);
}

int Adapter::get_item_count() {
	int result = 0;
	GDVIRTUAL_CALL(_get_item_count, result);
	return result;
}

int Adapter::get_item_view_type(int p_position) {
	int result = 0;
	GDVIRTUAL_CALL(_get_item_view_type, p_position, result);
	return result;
}

int Adapter::get_item_height(int p_position) {
	int result = -1;
	GDVIRTUAL_CALL(_get_item_height, p_position, result);
	return result;
}

int64_t Adapter::get_item_id(int p_position) {
	int64_t result = NO_ID;
	GDVIRTUAL_CALL(_get_item_id, p_position, result);
	return result;
}

void Adapter::set_has_stable_ids(bool p_has_stable_ids) {
	if (has_observers()) {
		ERR_PRINT("Cannot change whether this adapter has stable IDs while the adapter has registered observers.");
		return;
	}
	m_has_stable_ids = p_has_stable_ids;
}

void Adapter::register_adapter_data_observer(const Ref<AdapterDataObserver> &p_observer) {
	ERR_FAIL_NULL(p_observer);
	if (!m_observers.has(p_observer)) {
		m_observers.push_back(p_observer);
	}
}

void Adapter::unregister_adapter_data_observer(const Ref<AdapterDataObserver> &p_observer) {
	m_observers.erase(p_observer);
}

void Adapter::notify_data_set_changed() {
	for (const Ref<AdapterDataObserver> &obs : m_observers) {
		obs->on_changed();
	}
}

void Adapter::notify_item_range_changed(int p_position, int p_count, const Variant &p_payload) {
	for (const Ref<AdapterDataObserver> &obs : m_observers) {
		obs->on_item_range_changed(p_position, p_count, p_payload);
	}
}

void Adapter::notify_item_range_inserted(int p_position, int p_count) {
	for (const Ref<AdapterDataObserver> &obs : m_observers) {
		obs->on_item_range_inserted(p_position, p_count);
	}
}

void Adapter::notify_item_range_removed(int p_position, int p_count) {
	for (const Ref<AdapterDataObserver> &obs : m_observers) {
		obs->on_item_range_removed(p_position, p_count);
	}
}

void Adapter::notify_item_moved(int p_from_position, int p_to_position) {
	for (const Ref<AdapterDataObserver> &obs : m_observers) {
		obs->on_item_moved(p_from_position, p_to_position);
	}
}

void Adapter::notify_item_changed(int p_position) {
	notify_item_range_changed(p_position, 1, Variant());
}

void Adapter::notify_item_inserted(int p_position) {
	notify_item_range_inserted(p_position, 1);
}

void Adapter::notify_item_removed(int p_position) {
	notify_item_range_removed(p_position, 1);
}

} // namespace godot
