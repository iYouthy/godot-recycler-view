#pragma once

namespace godot {

// Port of androidx.recyclerview.widget.AdapterHelper.UpdateOp.
struct UpdateOp {
	enum {
		ADD = 1,
		REMOVE = 1 << 1,
		UPDATE = 1 << 2,
		MOVE = 1 << 3,
		POOL_SIZE = 30,
	};

	int cmd;
	int position_start;
	const void *payload;
	// Holds the target position if this is a MOVE.
	int item_count;

	UpdateOp() :
			cmd(0), position_start(0), payload(nullptr), item_count(0) {}

	UpdateOp(int p_cmd, int p_position_start, int p_item_count, const void *p_payload) :
			cmd(p_cmd), position_start(p_position_start), payload(p_payload), item_count(p_item_count) {}

	bool operator==(const UpdateOp &p_other) const {
		if (cmd != p_other.cmd) {
			return false;
		}
		if (cmd == MOVE && (item_count - position_start < 0 ? -(item_count - position_start) : item_count - position_start) == 1) {
			// Reverse of this is also the same move.
			if (item_count == p_other.position_start && position_start == p_other.item_count) {
				return true;
			}
		}
		if (item_count != p_other.item_count) {
			return false;
		}
		if (position_start != p_other.position_start) {
			return false;
		}
		if (payload != p_other.payload) {
			return false;
		}
		return true;
	}
};

} // namespace godot
