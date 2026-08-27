#pragma once

#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Port of RecyclerView.State. Carries layout state between passes and is the
// source of truth for the item count during a layout.
class State : public RefCounted {
	GDCLASS(State, RefCounted)

protected:
	static void _bind_methods();

public:
	enum LayoutStep {
		STEP_START = 1,
		STEP_LAYOUT = 1 << 1,
		STEP_ANIMATIONS = 1 << 2,
	};

	int get_target_scroll_position() const { return m_target_position; }
	bool has_target_scroll_position() const { return m_target_position != NO_POSITION; }
	void set_target_scroll_position(int p_position) { m_target_position = p_position; }

	bool is_measuring() const { return m_is_measuring; }
	void set_measuring(bool p_measuring) { m_is_measuring = p_measuring; }

	bool is_pre_layout() const { return m_in_pre_layout; }
	void set_pre_layout(bool p_in_pre_layout) { m_in_pre_layout = p_in_pre_layout; }

	bool will_run_predictive_animations() const { return m_run_predictive_animations; }
	void set_run_predictive_animations(bool p_value) { m_run_predictive_animations = p_value; }

	bool will_run_simple_animations() const { return m_run_simple_animations; }
	void set_run_simple_animations(bool p_value) { m_run_simple_animations = p_value; }

	bool did_structure_change() const { return m_structure_changed; }
	void set_structure_changed(bool p_changed) { m_structure_changed = p_changed; }

	int get_item_count() const {
		return m_in_pre_layout ? (m_previous_layout_item_count - m_deleted_invisible_item_count_since_previous_layout) : m_item_count;
	}
	void set_item_count(int p_count) { m_item_count = p_count; }

	int get_previous_layout_item_count() const { return m_previous_layout_item_count; }
	void set_previous_layout_item_count(int p_count) { m_previous_layout_item_count = p_count; }
	int get_deleted_invisible_item_count_since_previous_layout() const { return m_deleted_invisible_item_count_since_previous_layout; }
	void set_deleted_invisible_item_count_since_previous_layout(int p_count) { m_deleted_invisible_item_count_since_previous_layout = p_count; }

	int get_layout_step() const { return m_layout_step; }
	void set_layout_step(int p_step) { m_layout_step = p_step; }

	void prepare_for_nested_prefetch(int p_item_count) {
		m_layout_step = STEP_START;
		m_item_count = p_item_count;
		m_in_pre_layout = false;
		m_is_measuring = false;
	}

private:
	int m_target_position = NO_POSITION;
	int m_previous_layout_item_count = 0;
	int m_deleted_invisible_item_count_since_previous_layout = 0;
	int m_layout_step = STEP_START;
	int m_item_count = 0;
	bool m_structure_changed = false;
	bool m_in_pre_layout = false;
	bool m_is_measuring = false;
	bool m_run_simple_animations = false;
	bool m_run_predictive_animations = false;
};

} // namespace godot
