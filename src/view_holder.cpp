#include "view_holder.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void ViewHolder::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_control", "control"), &ViewHolder::set_control);
	ClassDB::bind_method(D_METHOD("get_control"), &ViewHolder::get_control);
	ClassDB::bind_method(D_METHOD("get_item_view_type"), &ViewHolder::get_item_view_type);
	ClassDB::bind_method(D_METHOD("set_item_view_type", "type"), &ViewHolder::set_item_view_type);
	ClassDB::bind_method(D_METHOD("get_stable_id"), &ViewHolder::get_stable_id);
	ClassDB::bind_method(D_METHOD("set_stable_id", "id"), &ViewHolder::set_stable_id);
	ClassDB::bind_method(D_METHOD("get_layout_position"), &ViewHolder::get_layout_position);
	ClassDB::bind_method(D_METHOD("get_position"), &ViewHolder::get_position);
	ClassDB::bind_method(D_METHOD("set_position", "position"), &ViewHolder::set_position);
	ClassDB::bind_method(D_METHOD("get_old_position"), &ViewHolder::get_old_position);
	ClassDB::bind_method(D_METHOD("is_bound"), &ViewHolder::is_bound);
	ClassDB::bind_method(D_METHOD("is_updated"), &ViewHolder::is_updated);
	ClassDB::bind_method(D_METHOD("is_invalid"), &ViewHolder::is_invalid);
	ClassDB::bind_method(D_METHOD("is_removed"), &ViewHolder::is_removed);
	ClassDB::bind_method(D_METHOD("should_ignore"), &ViewHolder::should_ignore);
	ClassDB::bind_method(D_METHOD("is_tmp_detached"), &ViewHolder::is_tmp_detached);
	ClassDB::bind_method(D_METHOD("is_adapter_position_unknown"), &ViewHolder::is_adapter_position_unknown);
	ClassDB::bind_method(D_METHOD("is_recyclable"), &ViewHolder::is_recyclable);
	ClassDB::bind_method(D_METHOD("set_is_recyclable", "recyclable"), &ViewHolder::set_is_recyclable);
	ClassDB::bind_method(D_METHOD("offset_position", "offset", "apply_to_pre_layout"), &ViewHolder::offset_position);
	ClassDB::bind_method(D_METHOD("clear_old_position"), &ViewHolder::clear_old_position);
	ClassDB::bind_method(D_METHOD("save_old_position"), &ViewHolder::save_old_position);
	ClassDB::bind_method(D_METHOD("set_flags", "flags", "mask"), &ViewHolder::set_flags);
	ClassDB::bind_method(D_METHOD("add_flags", "flags"), &ViewHolder::add_flags);
	ClassDB::bind_method(D_METHOD("get_flags"), &ViewHolder::get_flags);
	ClassDB::bind_method(D_METHOD("reset_internal"), &ViewHolder::reset_internal);
	ClassDB::bind_method(D_METHOD("flag_removed_and_offset_position", "new_position", "offset", "apply_to_pre_layout"), &ViewHolder::flag_removed_and_offset_position);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "control", PROPERTY_HINT_NONE, "Control", PROPERTY_USAGE_DEFAULT), "set_control", "get_control");

	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_BOUND", FLAG_BOUND);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_UPDATE", FLAG_UPDATE);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_INVALID", FLAG_INVALID);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_REMOVED", FLAG_REMOVED);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_NOT_RECYCLABLE", FLAG_NOT_RECYCLABLE);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_RETURNED_FROM_SCRAP", FLAG_RETURNED_FROM_SCRAP);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_IGNORE", FLAG_IGNORE);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_TMP_DETACHED", FLAG_TMP_DETACHED);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_ADAPTER_POSITION_UNKNOWN", FLAG_ADAPTER_POSITION_UNKNOWN);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_ADAPTER_FULLUPDATE", FLAG_ADAPTER_FULLUPDATE);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_MOVED", FLAG_MOVED);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_APPEARED_IN_PRE_LAYOUT", FLAG_APPEARED_IN_PRE_LAYOUT);
	ClassDB::bind_integer_constant(get_class_static(), "Flag", "FLAG_BOUNCED_FROM_HIDDEN_LIST", FLAG_BOUNCED_FROM_HIDDEN_LIST);
}

ViewHolder::ViewHolder() {}

ViewHolder::ViewHolder(Control *p_control) :
		m_control(p_control) {}

ViewHolder::~ViewHolder() {}

void ViewHolder::set_control(Control *p_control) {
	m_control = p_control;
}

Control *ViewHolder::get_control() const {
	return m_control;
}

void ViewHolder::set_is_recyclable(bool p_recyclable) {
	m_is_recyclable_count = p_recyclable ? m_is_recyclable_count - 1 : m_is_recyclable_count + 1;
	if (m_is_recyclable_count < 0) {
		m_is_recyclable_count = 0;
	}
	if (p_recyclable) {
		m_flags &= ~FLAG_NOT_RECYCLABLE;
	} else {
		m_flags |= FLAG_NOT_RECYCLABLE;
	}
}

void ViewHolder::flag_removed_and_offset_position(int p_new_position, int p_offset, bool p_apply_to_pre_layout) {
	add_flags(FLAG_REMOVED);
	offset_position(p_offset, p_apply_to_pre_layout);
	m_position = p_new_position;
}

void ViewHolder::offset_position(int p_offset, bool p_apply_to_pre_layout) {
	if (m_old_position == NO_POSITION) {
		m_old_position = m_position;
	}
	if (m_pre_layout_position == NO_POSITION) {
		m_pre_layout_position = m_position;
	}
	if (p_apply_to_pre_layout) {
		m_pre_layout_position += p_offset;
	}
	m_position += p_offset;
}

void ViewHolder::clear_old_position() {
	m_old_position = NO_POSITION;
	m_pre_layout_position = NO_POSITION;
}

void ViewHolder::save_old_position() {
	if (m_old_position == NO_POSITION) {
		m_old_position = m_position;
	}
}

void ViewHolder::set_flags(int p_flags, int p_mask) {
	m_flags = (m_flags & ~p_mask) | (p_flags & p_mask);
}

void ViewHolder::add_flags(int p_flags) {
	m_flags |= p_flags;
}

void ViewHolder::clear_payload() {
	// Payloads are introduced with item animations (Phase 3); nothing to clear yet.
}

void ViewHolder::reset_internal() {
	m_flags = 0;
	m_position = NO_POSITION;
	m_old_position = NO_POSITION;
	m_item_id = NO_ID;
	m_pre_layout_position = NO_POSITION;
	m_is_recyclable_count = 0;
	clear_payload();
}

} // namespace godot
