#include "item_touch_helper_callback.h"

#include "recycler_view.h"

namespace godot {

void ItemTouchHelperCallback::_bind_methods() {
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "UP", UP);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "DOWN", DOWN);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "LEFT", LEFT);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "RIGHT", RIGHT);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "START", START);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "END", END);

	GDVIRTUAL_BIND(_get_movement_flags, "holder");
	GDVIRTUAL_BIND(_is_long_press_drag_enabled);
	GDVIRTUAL_BIND(_is_item_view_swipe_enabled);
	GDVIRTUAL_BIND(_get_swipe_threshold, "holder");
	GDVIRTUAL_BIND(_get_move_threshold, "holder");
	GDVIRTUAL_BIND(_get_swipe_escape_velocity, "default");
	GDVIRTUAL_BIND(_get_swipe_velocity_threshold, "default");
	GDVIRTUAL_BIND(_get_bounding_box_margin);
	GDVIRTUAL_BIND(_can_drop_over, "selected", "target");
	GDVIRTUAL_BIND(_on_move, "recycler_view", "dragged", "target");
	GDVIRTUAL_BIND(_on_swiped, "holder", "direction");
	GDVIRTUAL_BIND(_on_selected_changed, "holder", "action_state");
	GDVIRTUAL_BIND(_clear_view, "holder");
}

int ItemTouchHelperCallback::get_movement_flags(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) {
	int result = 0;
	GDVIRTUAL_CALL(_get_movement_flags, p_holder, result);
	return result;
}

bool ItemTouchHelperCallback::is_long_press_drag_enabled() {
	bool result = true;
	GDVIRTUAL_CALL(_is_long_press_drag_enabled, result);
	return result;
}

bool ItemTouchHelperCallback::is_item_view_swipe_enabled() {
	bool result = true;
	GDVIRTUAL_CALL(_is_item_view_swipe_enabled, result);
	return result;
}

float ItemTouchHelperCallback::get_swipe_threshold(const Ref<ViewHolder> &p_holder) {
	float result = 0.5f;
	GDVIRTUAL_CALL(_get_swipe_threshold, p_holder, result);
	return result;
}

float ItemTouchHelperCallback::get_move_threshold(const Ref<ViewHolder> &p_holder) {
	float result = 0.5f;
	GDVIRTUAL_CALL(_get_move_threshold, p_holder, result);
	return result;
}

float ItemTouchHelperCallback::get_swipe_escape_velocity(float p_default) {
	float result = p_default;
	GDVIRTUAL_CALL(_get_swipe_escape_velocity, p_default, result);
	return result;
}

float ItemTouchHelperCallback::get_swipe_velocity_threshold(float p_default) {
	float result = p_default;
	GDVIRTUAL_CALL(_get_swipe_velocity_threshold, p_default, result);
	return result;
}

float ItemTouchHelperCallback::get_bounding_box_margin() {
	float result = 0.0f;
	GDVIRTUAL_CALL(_get_bounding_box_margin, result);
	return result;
}

bool ItemTouchHelperCallback::can_drop_over(const Ref<ViewHolder> &p_selected, const Ref<ViewHolder> &p_target) {
	bool result = true;
	GDVIRTUAL_CALL(_can_drop_over, p_selected, p_target, result);
	return result;
}

bool ItemTouchHelperCallback::on_move(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_dragged, const Ref<ViewHolder> &p_target) {
	bool result = false;
	GDVIRTUAL_CALL(_on_move, p_recycler_view, p_dragged, p_target, result);
	return result;
}

void ItemTouchHelperCallback::on_swiped(const Ref<ViewHolder> &p_holder, int p_direction) {
	GDVIRTUAL_CALL(_on_swiped, p_holder, p_direction);
}

void ItemTouchHelperCallback::on_selected_changed(const Ref<ViewHolder> &p_holder, int p_action_state) {
	GDVIRTUAL_CALL(_on_selected_changed, p_holder, p_action_state);
}

void ItemTouchHelperCallback::clear_view(const Ref<ViewHolder> &p_holder) {
	GDVIRTUAL_CALL(_clear_view, p_holder);
}

bool ItemTouchHelperCallback::has_drag_flag(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) {
	return (get_movement_flags(p_recycler_view, p_holder) & 0xFF0000) != 0;
}

bool ItemTouchHelperCallback::has_swipe_flag(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder) {
	return (get_movement_flags(p_recycler_view, p_holder) & 0xFF00) != 0;
}

} // namespace godot
