#include "op_reorderer.h"

namespace godot {

OpReorderer::OpReorderer(OpReordererCallback *p_callback) :
		m_callback(p_callback) {}

void OpReorderer::reorder_ops(Vector<UpdateOp *> &p_ops) {
	// Move operations break the continuity of add/remove handling, so push them
	// to the end of the list where they are handled easily.
	int bad_move;
	while ((bad_move = get_last_move_out_of_order(p_ops)) != -1) {
		swap_move_op(p_ops, bad_move, bad_move + 1);
	}
}

void OpReorderer::swap_move_op(Vector<UpdateOp *> &p_list, int p_bad_move, int p_next) {
	UpdateOp *move_op = p_list[p_bad_move];
	UpdateOp *next_op = p_list[p_next];
	switch (next_op->cmd) {
		case UpdateOp::REMOVE:
			swap_move_remove(p_list, p_bad_move, move_op, p_next, next_op);
			break;
		case UpdateOp::ADD:
			swap_move_add(p_list, p_bad_move, move_op, p_next, next_op);
			break;
		case UpdateOp::UPDATE:
			swap_move_update(p_list, p_bad_move, move_op, p_next, next_op);
			break;
	}
}

void OpReorderer::swap_move_remove(Vector<UpdateOp *> &p_list, int p_move_pos, UpdateOp *p_move_op, int p_remove_pos, UpdateOp *p_remove_op) {
	UpdateOp *extra_rm = nullptr;
	// Check if move is nulled out by remove.
	bool reverted_move = false;
	bool move_is_backwards;

	if (p_move_op->position_start < p_move_op->item_count) {
		move_is_backwards = false;
		if (p_remove_op->position_start == p_move_op->position_start
				&& p_remove_op->item_count == p_move_op->item_count - p_move_op->position_start) {
			reverted_move = true;
		}
	} else {
		move_is_backwards = true;
		if (p_remove_op->position_start == p_move_op->item_count + 1
				&& p_remove_op->item_count == p_move_op->position_start - p_move_op->item_count) {
			reverted_move = true;
		}
	}

	// Going in reverse, first revert the effect of add.
	if (p_move_op->item_count < p_remove_op->position_start) {
		p_remove_op->position_start--;
	} else if (p_move_op->item_count < p_remove_op->position_start + p_remove_op->item_count) {
		// Move is removed.
		p_remove_op->item_count--;
		p_move_op->cmd = UpdateOp::REMOVE;
		p_move_op->item_count = 1;
		if (p_remove_op->item_count == 0) {
			p_list.remove_at(p_remove_pos);
			m_callback->recycle_update_op(p_remove_op);
		}
		// No need to swap, it is already a remove.
		return;
	}

	// Now effect of add is consumed. Now apply effect of first remove.
	if (p_move_op->position_start <= p_remove_op->position_start) {
		p_remove_op->position_start++;
	} else if (p_move_op->position_start < p_remove_op->position_start + p_remove_op->item_count) {
		const int remaining = p_remove_op->position_start + p_remove_op->item_count - p_move_op->position_start;
		extra_rm = m_callback->obtain_update_op(UpdateOp::REMOVE, p_move_op->position_start + 1, remaining, nullptr);
		p_remove_op->item_count = p_move_op->position_start - p_remove_op->position_start;
	}

	// If effects of move are reverted by remove, we are done.
	if (reverted_move) {
		p_list.set(p_move_pos, p_remove_op);
		p_list.remove_at(p_remove_pos);
		m_callback->recycle_update_op(p_move_op);
		return;
	}

	// Now find out the new locations for move actions.
	if (move_is_backwards) {
		if (extra_rm != nullptr) {
			if (p_move_op->position_start > extra_rm->position_start) {
				p_move_op->position_start -= extra_rm->item_count;
			}
			if (p_move_op->item_count > extra_rm->position_start) {
				p_move_op->item_count -= extra_rm->item_count;
			}
		}
		if (p_move_op->position_start > p_remove_op->position_start) {
			p_move_op->position_start -= p_remove_op->item_count;
		}
		if (p_move_op->item_count > p_remove_op->position_start) {
			p_move_op->item_count -= p_remove_op->item_count;
		}
	} else {
		if (extra_rm != nullptr) {
			if (p_move_op->position_start >= extra_rm->position_start) {
				p_move_op->position_start -= extra_rm->item_count;
			}
			if (p_move_op->item_count >= extra_rm->position_start) {
				p_move_op->item_count -= extra_rm->item_count;
			}
		}
		if (p_move_op->position_start >= p_remove_op->position_start) {
			p_move_op->position_start -= p_remove_op->item_count;
		}
		if (p_move_op->item_count >= p_remove_op->position_start) {
			p_move_op->item_count -= p_remove_op->item_count;
		}
	}

	p_list.set(p_move_pos, p_remove_op);
	if (p_move_op->position_start != p_move_op->item_count) {
		p_list.set(p_remove_pos, p_move_op);
	} else {
		p_list.remove_at(p_remove_pos);
	}
	if (extra_rm != nullptr) {
		p_list.insert(p_move_pos, extra_rm);
	}
}

void OpReorderer::swap_move_add(Vector<UpdateOp *> &p_list, int p_move, UpdateOp *p_move_op, int p_add, UpdateOp *p_add_op) {
	int offset = 0;
	// Going in reverse, first revert the effect of add.
	if (p_move_op->item_count < p_add_op->position_start) {
		offset--;
	}
	if (p_move_op->position_start < p_add_op->position_start) {
		offset++;
	}
	if (p_add_op->position_start <= p_move_op->position_start) {
		p_move_op->position_start += p_add_op->item_count;
	}
	if (p_add_op->position_start <= p_move_op->item_count) {
		p_move_op->item_count += p_add_op->item_count;
	}
	p_add_op->position_start += offset;
	p_list.set(p_move, p_add_op);
	p_list.set(p_add, p_move_op);
}

void OpReorderer::swap_move_update(Vector<UpdateOp *> &p_list, int p_move, UpdateOp *p_move_op, int p_update, UpdateOp *p_update_op) {
	UpdateOp *extra_up1 = nullptr;
	UpdateOp *extra_up2 = nullptr;
	// Going in reverse, first revert the effect of add.
	if (p_move_op->item_count < p_update_op->position_start) {
		p_update_op->position_start--;
	} else if (p_move_op->item_count < p_update_op->position_start + p_update_op->item_count) {
		// Moved item is updated. Add an update for it.
		p_update_op->item_count--;
		extra_up1 = m_callback->obtain_update_op(UpdateOp::UPDATE, p_move_op->position_start, 1, p_update_op->payload);
	}
	// Now effect of add is consumed. Now apply effect of first remove.
	if (p_move_op->position_start <= p_update_op->position_start) {
		p_update_op->position_start++;
	} else if (p_move_op->position_start < p_update_op->position_start + p_update_op->item_count) {
		const int remaining = p_update_op->position_start + p_update_op->item_count - p_move_op->position_start;
		extra_up2 = m_callback->obtain_update_op(UpdateOp::UPDATE, p_move_op->position_start + 1, remaining, p_update_op->payload);
		p_update_op->item_count -= remaining;
	}
	p_list.set(p_update, p_move_op);
	if (p_update_op->item_count > 0) {
		p_list.set(p_move, p_update_op);
	} else {
		p_list.remove_at(p_move);
		m_callback->recycle_update_op(p_update_op);
	}
	if (extra_up1 != nullptr) {
		p_list.insert(p_move, extra_up1);
	}
	if (extra_up2 != nullptr) {
		p_list.insert(p_move, extra_up2);
	}
}

int OpReorderer::get_last_move_out_of_order(Vector<UpdateOp *> &p_list) const {
	bool found_non_move = false;
	for (int i = p_list.size() - 1; i >= 0; i--) {
		UpdateOp *op1 = p_list[i];
		if (op1->cmd == UpdateOp::MOVE) {
			if (found_non_move) {
				return i;
			}
		} else {
			found_non_move = true;
		}
	}
	return -1;
}

} // namespace godot
