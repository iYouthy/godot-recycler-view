#pragma once

#include "update_op.h"
#include "update_op_apply.h"
#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of androidx AdapterHelper, restricted to the one-pass path
// (consumeUpdatesInOnePass). Queues adapter change ops and applies the position
// offsets to attached ViewHolders when the layout runs. The two-phase
// pre/post-layout machinery used for animations is deferred.
class AdapterHelper : public RefCounted {
	GDCLASS(AdapterHelper, RefCounted)

protected:
	static void _bind_methods();

public:
	AdapterHelper();

	// Returns true if an op was queued (false for empty ranges / no-op moves).
	bool on_item_range_changed(int p_position_start, int p_item_count, const Variant &p_payload);
	bool on_item_range_inserted(int p_position_start, int p_item_count);
	bool on_item_range_removed(int p_position_start, int p_item_count);
	bool on_item_range_moved(int p_from_position, int p_to_position);

	bool has_pending_updates() const { return m_pending_ops.size() > 0; }
	void clear();

	// Applies all queued ops to a single holder (position transform + flags).
	void apply_updates_to_holder(const Ref<ViewHolder> &p_holder);

	// Applies queued ops to every attached holder, then clears the queue.
	// C++-only (takes a Vector by reference).
	void consume_updates_in_one_pass(Vector<Ref<ViewHolder>> &p_attached);

	// Maps a pre-update position to the current adapter position.
	int apply_pending_updates_to_position(int p_position) const;

	// The queued ops (consumed by consume_updates_in_one_pass).
	const Vector<UpdateOp> &get_pending_ops() const { return m_pending_ops; }

	// Returns the change payload for the given post-consume position, or an
	// empty Variant when the position was not updated with a payload. Filled by
	// consume_updates_in_one_pass: an UPDATE op's start is the new-list position,
	// which is exactly the position an attached holder has after the consume.
	Variant get_payload_at_position(int p_position) const;

private:
	Vector<UpdateOp> m_pending_ops;
	// Keeps payload Variants alive; op.payload stores (index + 1) as an opaque
	// handle so vector reallocation never invalidates it.
	Vector<Variant> m_payloads;
	// Parallel arrays: (post-consume position -> payload) for every item updated
	// with a change payload this cycle. Consumed by get_payload_at_position.
	Vector<int> m_update_payload_positions;
	Vector<Variant> m_update_payload_values;
};

} // namespace godot
