#pragma once

#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

class RecyclerView;

// Port of ItemTouchHelper.Callback. Scripts subclass this and override the
// hooks to declare per-holder drag/swipe flags and act on move/swipe commits.
class ItemTouchHelperCallback : public RefCounted {
	GDCLASS(ItemTouchHelperCallback, RefCounted)

protected:
	static void _bind_methods();

public:
	// Direction constants (ItemTouchHelper.Callback). START/END are relative
	// directions; this port is LTR-only so they map to LEFT/RIGHT.
	enum Direction {
		UP = 1,
		DOWN = 1 << 1,
		LEFT = 1 << 2,
		RIGHT = 1 << 3,
		START = LEFT << 2,
		END = RIGHT << 2,
	};

	// Script-overridable virtuals, mirroring ItemTouchHelper.Callback.
	GDVIRTUAL1R(int, _get_movement_flags, const Ref<ViewHolder> &)
	GDVIRTUAL0R(bool, _is_long_press_drag_enabled)
	GDVIRTUAL0R(bool, _is_item_view_swipe_enabled)
	GDVIRTUAL1R(float, _get_swipe_threshold, const Ref<ViewHolder> &)
	GDVIRTUAL1R(float, _get_move_threshold, const Ref<ViewHolder> &)
	GDVIRTUAL1R(float, _get_swipe_escape_velocity, float)
	GDVIRTUAL1R(float, _get_swipe_velocity_threshold, float)
	GDVIRTUAL0R(float, _get_bounding_box_margin)
	GDVIRTUAL2R(bool, _can_drop_over, const Ref<ViewHolder> &, const Ref<ViewHolder> &)
	// The recycler_view param is Object* (not RecyclerView*) so this header
	// needs no full RecyclerView definition (including it would be circular);
	// the C++ wrapper casts back to RecyclerView.
	GDVIRTUAL3R(bool, _on_move, Object *, const Ref<ViewHolder> &, const Ref<ViewHolder> &)
	GDVIRTUAL2(_on_swiped, const Ref<ViewHolder> &, int)
	GDVIRTUAL2(_on_selected_changed, const Ref<ViewHolder> &, int)
	GDVIRTUAL1(_clear_view, const Ref<ViewHolder> &)

	// C++ wrappers called by ItemTouchHelper.
	int get_movement_flags(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder);
	bool is_long_press_drag_enabled();
	bool is_item_view_swipe_enabled();
	float get_swipe_threshold(const Ref<ViewHolder> &p_holder);
	float get_move_threshold(const Ref<ViewHolder> &p_holder);
	float get_swipe_escape_velocity(float p_default);
	float get_swipe_velocity_threshold(float p_default);
	float get_bounding_box_margin();
	bool can_drop_over(const Ref<ViewHolder> &p_selected, const Ref<ViewHolder> &p_target);
	bool on_move(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_dragged, const Ref<ViewHolder> &p_target);
	void on_swiped(const Ref<ViewHolder> &p_holder, int p_direction);
	void on_selected_changed(const Ref<ViewHolder> &p_holder, int p_action_state);
	void clear_view(const Ref<ViewHolder> &p_holder);

	bool has_drag_flag(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder);
	bool has_swipe_flag(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder);
};

} // namespace godot
