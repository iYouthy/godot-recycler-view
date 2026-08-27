#include "update_op_apply.h"

namespace godot {

HolderUpdateEffect apply_update_op_to_holder(const UpdateOp &p_op, int p_position) {
	HolderUpdateEffect effect;
	effect.position = p_position;
	effect.removed = false;
	effect.updated = false;

	switch (p_op.cmd) {
		case UpdateOp::ADD: {
			if (p_position >= p_op.position_start) {
				effect.position = p_position + p_op.item_count;
			}
			break;
		}
		case UpdateOp::REMOVE: {
			const int end = p_op.position_start + p_op.item_count;
			if (p_position >= end) {
				effect.position = p_position - p_op.item_count;
			} else if (p_position >= p_op.position_start) {
				effect.position = POSITION_REMOVED;
				effect.removed = true;
			}
			break;
		}
		case UpdateOp::MOVE: {
			const int from = p_op.position_start;
			const int to = p_op.item_count;
			if (p_position == from) {
				effect.position = to;
			} else if (from < to) {
				if (p_position > from && p_position <= to) {
					effect.position = p_position - 1;
				}
			} else {
				if (p_position >= to && p_position < from) {
					effect.position = p_position + 1;
				}
			}
			break;
		}
		case UpdateOp::UPDATE: {
			if (p_position >= p_op.position_start && p_position < p_op.position_start + p_op.item_count) {
				effect.updated = true;
			}
			break;
		}
		default:
			break;
	}
	return effect;
}

int apply_pending_updates_to_position(const Vector<UpdateOp> &p_ops, int p_position) {
	int position = p_position;
	for (int i = 0; i < p_ops.size(); i++) {
		const UpdateOp &op = p_ops[i];
		switch (op.cmd) {
			case UpdateOp::ADD:
				if (op.position_start <= position) {
					position += op.item_count;
				}
				break;
			case UpdateOp::REMOVE: {
				if (op.position_start <= position) {
					const int end = op.position_start + op.item_count;
					if (end > position) {
						return POSITION_REMOVED;
					}
					position -= op.item_count;
				}
				break;
			}
			case UpdateOp::MOVE: {
				if (op.position_start == position) {
					position = op.item_count;
				} else {
					if (op.position_start < position) {
						position -= 1;
					}
					if (op.item_count <= position) {
						position += 1;
					}
				}
				break;
			}
			default:
				break;
		}
	}
	return position;
}

} // namespace godot
