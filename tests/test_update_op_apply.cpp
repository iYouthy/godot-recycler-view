// Pure tests for the adapter update position math (update_op_apply).
// Covers the per-holder transforms behind AdapterHelper.consumeUpdatesInOnePass
// and the reverse mapping used for scrap matching.

#include "doctest.h"

#include "update_op_apply.h"

using namespace godot;

TEST_CASE("apply_update_op_to_holder: ADD shifts positions at and after start") {
	UpdateOp add(UpdateOp::ADD, 3, 2, nullptr);
	CHECK(apply_update_op_to_holder(add, 0).position == 0);
	CHECK(apply_update_op_to_holder(add, 2).position == 2);
	CHECK(apply_update_op_to_holder(add, 3).position == 5);
	CHECK(apply_update_op_to_holder(add, 7).position == 9);
	CHECK(!apply_update_op_to_holder(add, 7).removed);
	CHECK(!apply_update_op_to_holder(add, 7).updated);
}

TEST_CASE("apply_update_op_to_holder: REMOVE shifts after, removes in range") {
	UpdateOp rm(UpdateOp::REMOVE, 2, 3, nullptr);
	// Below the removed range: unchanged.
	CHECK(apply_update_op_to_holder(rm, 0).position == 0);
	CHECK(apply_update_op_to_holder(rm, 1).position == 1);
	// Inside the removed range: marked removed.
	HolderUpdateEffect in_range = apply_update_op_to_holder(rm, 3);
	CHECK(in_range.position == POSITION_REMOVED);
	CHECK(in_range.removed);
	// After the removed range: shifted down.
	CHECK(apply_update_op_to_holder(rm, 5).position == 2);
	CHECK(apply_update_op_to_holder(rm, 9).position == 6);
}

TEST_CASE("apply_update_op_to_holder: MOVE forward shifts in-between down") {
	UpdateOp mv(UpdateOp::MOVE, 2, 5, nullptr); // from=2, to=5
	CHECK(apply_update_op_to_holder(mv, 2).position == 5); // the moved item
	CHECK(apply_update_op_to_holder(mv, 3).position == 2); // in-between -1
	CHECK(apply_update_op_to_holder(mv, 4).position == 3);
	CHECK(apply_update_op_to_holder(mv, 5).position == 4);
	CHECK(apply_update_op_to_holder(mv, 1).position == 1); // before: unchanged
	CHECK(apply_update_op_to_holder(mv, 6).position == 6); // after: unchanged
}

TEST_CASE("apply_update_op_to_holder: MOVE backward shifts in-between up") {
	UpdateOp mv(UpdateOp::MOVE, 5, 2, nullptr); // from=5, to=2
	CHECK(apply_update_op_to_holder(mv, 5).position == 2); // the moved item
	CHECK(apply_update_op_to_holder(mv, 2).position == 3); // in-between +1
	CHECK(apply_update_op_to_holder(mv, 3).position == 4);
	CHECK(apply_update_op_to_holder(mv, 4).position == 5);
	CHECK(apply_update_op_to_holder(mv, 1).position == 1); // before: unchanged
	CHECK(apply_update_op_to_holder(mv, 6).position == 6); // after: unchanged
}

TEST_CASE("apply_update_op_to_holder: UPDATE marks range without moving") {
	UpdateOp up(UpdateOp::UPDATE, 1, 3, nullptr);
	HolderUpdateEffect hit = apply_update_op_to_holder(up, 2);
	CHECK(hit.position == 2);
	CHECK(hit.updated);
	CHECK(!hit.removed);
	CHECK(!apply_update_op_to_holder(up, 0).updated);
	CHECK(!apply_update_op_to_holder(up, 4).updated);
}

TEST_CASE("apply_pending_updates_to_position: reverse mapping") {
	// Maps a pre-update position to the post-update position (Android:
	// AdapterHelper.applyPendingUpdatesToPosition). Used to resolve a holder's
	// layout position to the current adapter position for scrap matching.
	// A single insert of 2 at 3: positions at/after the insert shift right.
	{
		Vector<UpdateOp> ops;
		ops.push_back(UpdateOp(UpdateOp::ADD, 3, 2, nullptr));
		CHECK(apply_pending_updates_to_position(ops, 0) == 0);
		CHECK(apply_pending_updates_to_position(ops, 2) == 2);
		CHECK(apply_pending_updates_to_position(ops, 3) == 5);
		CHECK(apply_pending_updates_to_position(ops, 4) == 6);
		CHECK(apply_pending_updates_to_position(ops, 9) == 11);
	}

	// A single remove of 2 at 3: positions in the range are gone, after shifts left.
	{
		Vector<UpdateOp> ops;
		ops.push_back(UpdateOp(UpdateOp::REMOVE, 3, 2, nullptr));
		CHECK(apply_pending_updates_to_position(ops, 0) == 0);
		CHECK(apply_pending_updates_to_position(ops, 2) == 2);
		CHECK(apply_pending_updates_to_position(ops, 3) == POSITION_REMOVED);
		CHECK(apply_pending_updates_to_position(ops, 4) == POSITION_REMOVED);
		CHECK(apply_pending_updates_to_position(ops, 5) == 3);
		CHECK(apply_pending_updates_to_position(ops, 7) == 5);
	}

	// A single move 2 -> 5.
	{
		Vector<UpdateOp> ops;
		ops.push_back(UpdateOp(UpdateOp::MOVE, 2, 5, nullptr));
		CHECK(apply_pending_updates_to_position(ops, 2) == 5); // moved item
		CHECK(apply_pending_updates_to_position(ops, 3) == 2); // shifted down
		CHECK(apply_pending_updates_to_position(ops, 5) == 4);
		CHECK(apply_pending_updates_to_position(ops, 0) == 0);
	}
}

TEST_CASE("apply_pending_updates_to_position: op sequences") {
	// Insert 2 at 0, then remove 1 at 4 (ops applied in order).
	{
		Vector<UpdateOp> ops;
		ops.push_back(UpdateOp(UpdateOp::ADD, 0, 2, nullptr));
		ops.push_back(UpdateOp(UpdateOp::REMOVE, 4, 1, nullptr));
		// pre 0 -> add 2 -> 2 (remove at 4 doesn't affect 2).
		CHECK(apply_pending_updates_to_position(ops, 0) == 2);
		// pre 4 -> add 2 -> 6; remove: 4 <= 6, end 5 > 6? no -> 6-1 = 5.
		CHECK(apply_pending_updates_to_position(ops, 4) == 5);
		// pre 3 -> add 2 -> 5; remove: 4 <= 5, end 5 > 5? no -> 4.
		CHECK(apply_pending_updates_to_position(ops, 3) == 4);
	}

	// Remove 1 at 4, then insert 2 at 0.
	{
		Vector<UpdateOp> ops;
		ops.push_back(UpdateOp(UpdateOp::REMOVE, 4, 1, nullptr));
		ops.push_back(UpdateOp(UpdateOp::ADD, 0, 2, nullptr));
		// pre 0 -> remove no-op -> 0; add -> 2.
		CHECK(apply_pending_updates_to_position(ops, 0) == 2);
		// pre 4 -> removed by the REMOVE.
		CHECK(apply_pending_updates_to_position(ops, 4) == POSITION_REMOVED);
		// pre 3 -> remove no-op -> 3; add -> 5.
		CHECK(apply_pending_updates_to_position(ops, 3) == 5);
		// pre 5 -> remove -> 4; add -> 6.
		CHECK(apply_pending_updates_to_position(ops, 5) == 6);
	}
}

TEST_CASE("apply_pending_updates_to_position: removed position returns sentinel") {
	Vector<UpdateOp> ops;
	ops.push_back(UpdateOp(UpdateOp::REMOVE, 2, 3, nullptr));
	// Pre-update positions inside the removed block map to the sentinel.
	CHECK(apply_pending_updates_to_position(ops, 2) == POSITION_REMOVED);
	CHECK(apply_pending_updates_to_position(ops, 3) == POSITION_REMOVED);
	CHECK(apply_pending_updates_to_position(ops, 4) == POSITION_REMOVED);
	// Outside the block map normally.
	CHECK(apply_pending_updates_to_position(ops, 0) == 0);
	CHECK(apply_pending_updates_to_position(ops, 1) == 1);
	CHECK(apply_pending_updates_to_position(ops, 5) == 2);
}
