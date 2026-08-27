#pragma once

#include "update_op.h"

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.OpReorderer. Reorders a batch of
// adapter update operations so that MOVE operations are pushed to the end,
// resolving the effects of adds/removes/updates they cross.
class OpReordererCallback {
public:
	virtual ~OpReordererCallback() = default;

	virtual UpdateOp *obtain_update_op(int p_cmd, int p_start_position, int p_item_count, const void *p_payload) = 0;
	virtual void recycle_update_op(UpdateOp *p_op) = 0;
};

class OpReorderer {
public:
	explicit OpReorderer(OpReordererCallback *p_callback);

	void reorder_ops(Vector<UpdateOp *> &p_ops);

	void swap_move_remove(Vector<UpdateOp *> &p_list, int p_move_pos, UpdateOp *p_move_op, int p_remove_pos, UpdateOp *p_remove_op);
	void swap_move_add(Vector<UpdateOp *> &p_list, int p_move, UpdateOp *p_move_op, int p_add, UpdateOp *p_add_op);
	void swap_move_update(Vector<UpdateOp *> &p_list, int p_move, UpdateOp *p_move_op, int p_update, UpdateOp *p_update_op);

private:
	void swap_move_op(Vector<UpdateOp *> &p_list, int p_bad_move, int p_next);
	int get_last_move_out_of_order(Vector<UpdateOp *> &p_list) const;

	OpReordererCallback *m_callback;
};

} // namespace godot
