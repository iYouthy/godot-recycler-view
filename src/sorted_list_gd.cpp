#include "sorted_list_gd.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void SortedListCallback::_bind_methods() {
	GDVIRTUAL_BIND(_compare, "o1", "o2");
	GDVIRTUAL_BIND(_are_items_the_same, "item1", "item2");
	GDVIRTUAL_BIND(_are_contents_the_same, "old_item", "new_item");
	GDVIRTUAL_BIND(_get_change_payload, "item1", "item2");
	GDVIRTUAL_BIND(_on_inserted, "position", "count");
	GDVIRTUAL_BIND(_on_removed, "position", "count");
	GDVIRTUAL_BIND(_on_moved, "from_position", "to_position");
	GDVIRTUAL_BIND(_on_changed, "position", "count");
	GDVIRTUAL_BIND(_on_changed_with_payload, "position", "count", "payload");
}

int SortedListCallback::compare(const Variant &p_o1, const Variant &p_o2) {
	int result = 0;
	GDVIRTUAL_CALL(_compare, p_o1, p_o2, result);
	return result;
}

bool SortedListCallback::are_items_the_same(const Variant &p_item1, const Variant &p_item2) {
	bool result = false;
	GDVIRTUAL_CALL(_are_items_the_same, p_item1, p_item2, result);
	return result;
}

bool SortedListCallback::are_contents_the_same(const Variant &p_old_item, const Variant &p_new_item) {
	bool result = false;
	GDVIRTUAL_CALL(_are_contents_the_same, p_old_item, p_new_item, result);
	return result;
}

const void *SortedListCallback::get_change_payload(const Variant &p_item1, const Variant &p_item2) {
	Variant result;
	if (GDVIRTUAL_CALL(_get_change_payload, p_item1, p_item2, result)) {
		m_payload_buffer = result;
		return &m_payload_buffer;
	}
	return nullptr;
}

void SortedListCallback::on_inserted(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_inserted, p_position, p_count);
}

void SortedListCallback::on_removed(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_removed, p_position, p_count);
}

void SortedListCallback::on_moved(int p_from_position, int p_to_position) {
	GDVIRTUAL_CALL(_on_moved, p_from_position, p_to_position);
}

void SortedListCallback::on_changed(int p_position, int p_count, const void *p_payload) {
	if (GDVIRTUAL_IS_OVERRIDDEN(_on_changed_with_payload)) {
		const Variant *payload = static_cast<const Variant *>(p_payload);
		GDVIRTUAL_CALL(_on_changed_with_payload, p_position, p_count, payload ? *payload : Variant());
	} else {
		GDVIRTUAL_CALL(_on_changed, p_position, p_count);
	}
}

// ---------------------------------------------------------------------------
// SortedList.

void SortedList::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_callback", "callback"), &SortedList::set_callback);
	ClassDB::bind_method(D_METHOD("get_callback"), &SortedList::get_callback);
	ClassDB::bind_method(D_METHOD("size"), &SortedList::size);
	ClassDB::bind_method(D_METHOD("add", "item"), &SortedList::add);
	ClassDB::bind_method(D_METHOD("add_all", "items"), &SortedList::add_all);
	ClassDB::bind_method(D_METHOD("replace_all", "items"), &SortedList::replace_all);
	ClassDB::bind_method(D_METHOD("remove", "item"), &SortedList::remove);
	ClassDB::bind_method(D_METHOD("remove_item_at", "index"), &SortedList::remove_item_at);
	ClassDB::bind_method(D_METHOD("update_item_at", "index", "item"), &SortedList::update_item_at);
	ClassDB::bind_method(D_METHOD("recalculate_position_of_item_at", "index"), &SortedList::recalculate_position_of_item_at);
	ClassDB::bind_method(D_METHOD("get", "index"), &SortedList::get);
	ClassDB::bind_method(D_METHOD("index_of", "item"), &SortedList::index_of);
	ClassDB::bind_method(D_METHOD("clear"), &SortedList::clear);
	ClassDB::bind_method(D_METHOD("begin_batched_updates"), &SortedList::begin_batched_updates);
	ClassDB::bind_method(D_METHOD("end_batched_updates"), &SortedList::end_batched_updates);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "callback", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_callback", "get_callback"); // runtime ref, not scene-serializable
}

SortedList::SortedList() {}

SortedList::~SortedList() {
	if (m_core != nullptr) {
		memdelete(m_core);
	}
}

void SortedList::set_callback(const Ref<SortedListCallback> &p_callback) {
	ERR_FAIL_NULL(p_callback);
	if (m_core != nullptr) {
		memdelete(m_core);
		m_core = nullptr;
	}
	m_callback = p_callback;
	m_core = memnew(SortedListCore<Variant>(p_callback.ptr()));
}

Ref<SortedListCallback> SortedList::get_callback() const {
	return m_callback;
}

int SortedList::size() const {
	ERR_FAIL_NULL_V(m_core, 0);
	return m_core->size();
}

int SortedList::add(const Variant &p_item) {
	ERR_FAIL_NULL_V(m_core, INVALID_POSITION);
	return m_core->add(p_item);
}

void SortedList::add_all(const Array &p_items) {
	ERR_FAIL_NULL(m_core);
	Vector<Variant> items;
	items.resize(p_items.size());
	for (int i = 0; i < p_items.size(); i++) {
		items.write[i] = p_items[i];
	}
	m_core->add_all(items);
}

void SortedList::replace_all(const Array &p_items) {
	ERR_FAIL_NULL(m_core);
	Vector<Variant> items;
	items.resize(p_items.size());
	for (int i = 0; i < p_items.size(); i++) {
		items.write[i] = p_items[i];
	}
	m_core->replace_all(items);
}

bool SortedList::remove(const Variant &p_item) {
	ERR_FAIL_NULL_V(m_core, false);
	return m_core->remove(p_item);
}

Variant SortedList::remove_item_at(int p_index) {
	ERR_FAIL_NULL_V(m_core, Variant());
	return m_core->remove_item_at(p_index);
}

void SortedList::update_item_at(int p_index, const Variant &p_item) {
	ERR_FAIL_NULL(m_core);
	m_core->update_item_at(p_index, p_item);
}

void SortedList::recalculate_position_of_item_at(int p_index) {
	ERR_FAIL_NULL(m_core);
	m_core->recalculate_position_of_item_at(p_index);
}

Variant SortedList::get(int p_index) const {
	ERR_FAIL_NULL_V(m_core, Variant());
	if (p_index < 0 || p_index >= m_core->size()) {
		return Variant();
	}
	return m_core->get(p_index);
}

int SortedList::index_of(const Variant &p_item) const {
	ERR_FAIL_NULL_V(m_core, INVALID_POSITION);
	return m_core->index_of(p_item);
}

void SortedList::clear() {
	ERR_FAIL_NULL(m_core);
	m_core->clear();
}

void SortedList::begin_batched_updates() {
	ERR_FAIL_NULL(m_core);
	m_core->begin_batched_updates();
}

void SortedList::end_batched_updates() {
	ERR_FAIL_NULL(m_core);
	m_core->end_batched_updates();
}

} // namespace godot
