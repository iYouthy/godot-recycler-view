#include "list_update_callback.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void ListUpdateCallback::_bind_methods() {
	GDVIRTUAL_BIND(_on_inserted, "position", "count");
	GDVIRTUAL_BIND(_on_removed, "position", "count");
	GDVIRTUAL_BIND(_on_moved, "from_position", "to_position");
	GDVIRTUAL_BIND(_on_changed, "position", "count", "payload");
}

void ListUpdateCallback::on_inserted(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_inserted, p_position, p_count);
}

void ListUpdateCallback::on_removed(int p_position, int p_count) {
	GDVIRTUAL_CALL(_on_removed, p_position, p_count);
}

void ListUpdateCallback::on_moved(int p_from_position, int p_to_position) {
	GDVIRTUAL_CALL(_on_moved, p_from_position, p_to_position);
}

void ListUpdateCallback::on_changed(int p_position, int p_count, const Variant &p_payload) {
	GDVIRTUAL_CALL(_on_changed, p_position, p_count, p_payload);
}

// ---------------------------------------------------------------------------
// BatchingListUpdateCallback.

void BatchingListUpdateCallback::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_wrapped", "wrapped"), &BatchingListUpdateCallback::set_wrapped);
	ClassDB::bind_method(D_METHOD("get_wrapped"), &BatchingListUpdateCallback::get_wrapped);
	ClassDB::bind_method(D_METHOD("on_inserted", "position", "count"), &BatchingListUpdateCallback::on_inserted);
	ClassDB::bind_method(D_METHOD("on_removed", "position", "count"), &BatchingListUpdateCallback::on_removed);
	ClassDB::bind_method(D_METHOD("on_moved", "from_position", "to_position"), &BatchingListUpdateCallback::on_moved);
	ClassDB::bind_method(D_METHOD("on_changed", "position", "count", "payload"), &BatchingListUpdateCallback::on_changed);
	ClassDB::bind_method(D_METHOD("dispatch_last_event"), &BatchingListUpdateCallback::dispatch_last_event);
	ClassDB::bind_method(D_METHOD("get_last_event_type"), &BatchingListUpdateCallback::get_last_event_type);
	ClassDB::bind_method(D_METHOD("get_last_event_position"), &BatchingListUpdateCallback::get_last_event_position);
	ClassDB::bind_method(D_METHOD("get_last_event_count"), &BatchingListUpdateCallback::get_last_event_count);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wrapped", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT), "set_wrapped", "get_wrapped");
}

BatchingListUpdateCallback::BatchingListUpdateCallback(const Ref<ListUpdateCallback> &p_wrapped) :
		m_wrapped(p_wrapped) {}

void BatchingListUpdateCallback::set_wrapped(const Ref<ListUpdateCallback> &p_wrapped) {
	m_wrapped = p_wrapped;
}

Ref<ListUpdateCallback> BatchingListUpdateCallback::get_wrapped() const {
	return m_wrapped;
}

void BatchingListUpdateCallback::on_inserted(int p_position, int p_count) {
	if (m_last_event_type == TYPE_ADD && p_position >= m_last_event_position && p_position <= m_last_event_position + m_last_event_count) {
		m_last_event_count += p_count;
		m_last_event_position = p_position < m_last_event_position ? p_position : m_last_event_position;
		return;
	}
	dispatch_last_event();
	m_last_event_position = p_position;
	m_last_event_count = p_count;
	m_last_event_type = TYPE_ADD;
}

void BatchingListUpdateCallback::on_removed(int p_position, int p_count) {
	if (m_last_event_type == TYPE_REMOVE && m_last_event_position >= p_position && m_last_event_position <= p_position + p_count) {
		m_last_event_count += p_count;
		m_last_event_position = p_position;
		return;
	}
	dispatch_last_event();
	m_last_event_position = p_position;
	m_last_event_count = p_count;
	m_last_event_type = TYPE_REMOVE;
}

void BatchingListUpdateCallback::on_moved(int p_from_position, int p_to_position) {
	dispatch_last_event();
	m_last_event_position = p_from_position;
	m_last_event_count = p_to_position;
	m_last_event_type = TYPE_MOVE;
}

void BatchingListUpdateCallback::on_changed(int p_position, int p_count, const Variant &p_payload) {
	if (m_last_event_type == TYPE_CHANGE
			&& p_position <= m_last_event_position + m_last_event_count
			&& p_position + p_count >= m_last_event_position
			&& m_last_event_payload == p_payload) {
		m_last_event_position = p_position < m_last_event_position ? p_position : m_last_event_position;
		int end = p_position + p_count;
		m_last_event_count = end - m_last_event_position > m_last_event_count ? end - m_last_event_position : m_last_event_count;
		return;
	}
	dispatch_last_event();
	m_last_event_position = p_position;
	m_last_event_count = p_count;
	m_last_event_payload = p_payload;
	m_last_event_type = TYPE_CHANGE;
}

void BatchingListUpdateCallback::dispatch_last_event() {
	if (m_last_event_type == TYPE_NONE) {
		return;
	}
	switch (m_last_event_type) {
		case TYPE_ADD:
			m_wrapped->on_inserted(m_last_event_position, m_last_event_count);
			break;
		case TYPE_REMOVE:
			m_wrapped->on_removed(m_last_event_position, m_last_event_count);
			break;
		case TYPE_CHANGE:
			m_wrapped->on_changed(m_last_event_position, m_last_event_count, m_last_event_payload);
			break;
		case TYPE_MOVE:
			m_wrapped->on_moved(m_last_event_position, m_last_event_count);
			break;
	}
	reset_state();
}

void BatchingListUpdateCallback::reset_state() {
	m_last_event_type = TYPE_NONE;
	m_last_event_position = -1;
	m_last_event_count = -1;
	m_last_event_payload = Variant();
}

} // namespace godot
