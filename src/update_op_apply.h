#pragma once

#include "update_op.h"

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of the position math behind androidx AdapterHelper and
// RecyclerView.offsetPositionRecordsFor*. Pure functions over UpdateOps, no
// ViewHolder dependency (doctest-testable).

// Marks a position as removed by a REMOVE op.
constexpr int POSITION_REMOVED = -1;

// The effect of one update op on a single layout position.
struct HolderUpdateEffect {
	int position;
	bool removed;
	bool updated;
};

// Applies one update op to a layout position, mirroring
// RecyclerView.offsetPositionRecordsForInsert/Remove/Move.
HolderUpdateEffect apply_update_op_to_holder(const UpdateOp &p_op, int p_position);

// Maps a post-update position back through the pending ops to the position it
// had in the original list (Android: AdapterHelper.applyPendingUpdatesToPosition).
// Returns POSITION_REMOVED if the position was deleted by a REMOVE.
int apply_pending_updates_to_position(const Vector<UpdateOp> &p_ops, int p_position);

} // namespace godot
