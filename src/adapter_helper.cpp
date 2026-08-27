#include "adapter_helper.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void AdapterHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("on_item_range_changed", "position_start", "item_count", "payload"), &AdapterHelper::on_item_range_changed);
	ClassDB::bind_method(D_METHOD("on_item_range_inserted", "position_start", "item_count"), &AdapterHelper::on_item_range_inserted);
	ClassDB::bind_method(D_METHOD("on_item_range_removed", "position_start", "item_count"), &AdapterHelper::on_item_range_removed);
	ClassDB::bind_method(D_METHOD("on_item_range_moved", "from_position", "to_position"), &AdapterHelper::on_item_range_moved);
	ClassDB::bind_method(D_METHOD("has_pending_updates"), &AdapterHelper::has_pending_updates);
	ClassDB::bind_method(D_METHOD("clear"), &AdapterHelper::clear);
	ClassDB::bind_method(D_METHOD("apply_updates_to_holder", "holder"), &AdapterHelper::apply_updates_to_holder);
	ClassDB::bind_method(D_METHOD("apply_pending_updates_to_position", "position"), &AdapterHelper::apply_pending_updates_to_position);
}

AdapterHelper::AdapterHelper() {}

bool AdapterHelper::on_item_range_changed(int p_position_start, int p_item_count, const Variant &p_payload) {
	if (p_item_count < 1) {
		return false;
	}
	const int payload_index = m_payloads.size();
	m_payloads.push_back(p_payload);
	m_pending_ops.push_back(UpdateOp(UpdateOp::UPDATE, p_position_start, p_item_count, (const void *)(intptr_t)(payload_index + 1)));
	return m_pending_ops.size() == 1;
}

bool AdapterHelper::on_item_range_inserted(int p_position_start, int p_item_count) {
	if (p_item_count < 1) {
		return false;
	}
	m_pending_ops.push_back(UpdateOp(UpdateOp::ADD, p_position_start, p_item_count, nullptr));
	return m_pending_ops.size() == 1;
}

bool AdapterHelper::on_item_range_removed(int p_position_start, int p_item_count) {
	if (p_item_count < 1) {
		return false;
	}
	m_pending_ops.push_back(UpdateOp(UpdateOp::REMOVE, p_position_start, p_item_count, nullptr));
	return m_pending_ops.size() == 1;
}

bool AdapterHelper::on_item_range_moved(int p_from_position, int p_to_position) {
	if (p_from_position == p_to_position) {
		return false;
	}
	m_pending_ops.push_back(UpdateOp(UpdateOp::MOVE, p_from_position, p_to_position, nullptr));
	return m_pending_ops.size() == 1;
}

void AdapterHelper::clear() {
	m_pending_ops.clear();
	m_payloads.clear();
}

void AdapterHelper::apply_updates_to_holder(const Ref<ViewHolder> &p_holder) {
	ERR_FAIL_NULL(p_holder);
	int position = p_holder->get_position();
	bool removed = false;
	for (int i = 0; i < m_pending_ops.size(); i++) {
		if (removed) {
			break;
		}
		const HolderUpdateEffect effect = apply_update_op_to_holder(m_pending_ops[i], position);
		position = effect.position;
		if (effect.removed) {
			removed = true;
		}
		if (effect.updated) {
			p_holder->add_flags(ViewHolder::FLAG_UPDATE);
		}
	}
	if (removed) {
		p_holder->set_position(NO_POSITION);
		p_holder->add_flags(ViewHolder::FLAG_REMOVED);
	} else {
		p_holder->set_position(position);
	}
}

void AdapterHelper::consume_updates_in_one_pass(Vector<Ref<ViewHolder>> &p_attached) {
	if (m_pending_ops.is_empty()) {
		return;
	}
	// Collect the change payloads keyed by the post-consume position (an UPDATE
	// op's start is the new-list position, which is where its holder lands after
	// the consume). Consumed later by the RecyclerView's rebind pass.
	m_update_payload_positions.clear();
	m_update_payload_values.clear();
	for (int i = 0; i < m_pending_ops.size(); i++) {
		const UpdateOp &op = m_pending_ops[i];
		if (op.cmd == UpdateOp::UPDATE) {
			const int payload_index = (int)(intptr_t)op.payload - 1;
			const Variant payload = (payload_index >= 0 && payload_index < m_payloads.size()) ? m_payloads[payload_index] : Variant();
			for (int k = 0; k < op.item_count; k++) {
				m_update_payload_positions.push_back(op.position_start + k);
				m_update_payload_values.push_back(payload);
			}
		}
	}
	for (int i = 0; i < p_attached.size(); i++) {
		apply_updates_to_holder(p_attached[i]);
	}
	clear();
}

Variant AdapterHelper::get_payload_at_position(int p_position) const {
	for (int i = 0; i < m_update_payload_positions.size(); i++) {
		if (m_update_payload_positions[i] == p_position) {
			return m_update_payload_values[i];
		}
	}
	return Variant();
}

int AdapterHelper::apply_pending_updates_to_position(int p_position) const {
	return ::godot::apply_pending_updates_to_position(m_pending_ops, p_position);
}

} // namespace godot
