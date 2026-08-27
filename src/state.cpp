#include "state.h"

namespace godot {

void State::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_target_scroll_position"), &State::get_target_scroll_position);
	ClassDB::bind_method(D_METHOD("has_target_scroll_position"), &State::has_target_scroll_position);
	ClassDB::bind_method(D_METHOD("set_target_scroll_position", "position"), &State::set_target_scroll_position);
	ClassDB::bind_method(D_METHOD("is_measuring"), &State::is_measuring);
	ClassDB::bind_method(D_METHOD("set_measuring", "measuring"), &State::set_measuring);
	ClassDB::bind_method(D_METHOD("is_pre_layout"), &State::is_pre_layout);
	ClassDB::bind_method(D_METHOD("set_pre_layout", "in_pre_layout"), &State::set_pre_layout);
	ClassDB::bind_method(D_METHOD("will_run_predictive_animations"), &State::will_run_predictive_animations);
	ClassDB::bind_method(D_METHOD("set_run_predictive_animations", "value"), &State::set_run_predictive_animations);
	ClassDB::bind_method(D_METHOD("will_run_simple_animations"), &State::will_run_simple_animations);
	ClassDB::bind_method(D_METHOD("set_run_simple_animations", "value"), &State::set_run_simple_animations);
	ClassDB::bind_method(D_METHOD("did_structure_change"), &State::did_structure_change);
	ClassDB::bind_method(D_METHOD("set_structure_changed", "changed"), &State::set_structure_changed);
	ClassDB::bind_method(D_METHOD("get_item_count"), &State::get_item_count);
	ClassDB::bind_method(D_METHOD("set_item_count", "count"), &State::set_item_count);
	ClassDB::bind_method(D_METHOD("get_previous_layout_item_count"), &State::get_previous_layout_item_count);
	ClassDB::bind_method(D_METHOD("set_previous_layout_item_count", "count"), &State::set_previous_layout_item_count);
	ClassDB::bind_method(D_METHOD("get_deleted_invisible_item_count_since_previous_layout"), &State::get_deleted_invisible_item_count_since_previous_layout);
	ClassDB::bind_method(D_METHOD("set_deleted_invisible_item_count_since_previous_layout", "count"), &State::set_deleted_invisible_item_count_since_previous_layout);
	ClassDB::bind_method(D_METHOD("get_layout_step"), &State::get_layout_step);
	ClassDB::bind_method(D_METHOD("set_layout_step", "step"), &State::set_layout_step);
	ClassDB::bind_method(D_METHOD("prepare_for_nested_prefetch", "item_count"), &State::prepare_for_nested_prefetch);
}

} // namespace godot
