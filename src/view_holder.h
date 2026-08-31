#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Constants shared with RecyclerView (defined here until RecyclerView lands).
constexpr int NO_POSITION = -1;
constexpr int64_t NO_ID = -1;
constexpr int INVALID_TYPE = -1;

// Port of RecyclerView.ViewHolder. Wraps a Control that displays one item.
class ViewHolder : public RefCounted {
	GDCLASS(ViewHolder, RefCounted)

protected:
	static void _bind_methods();

public:
	enum Flag {
		FLAG_BOUND = 1 << 0,
		FLAG_UPDATE = 1 << 1,
		FLAG_INVALID = 1 << 2,
		FLAG_REMOVED = 1 << 3,
		FLAG_NOT_RECYCLABLE = 1 << 4,
		FLAG_RETURNED_FROM_SCRAP = 1 << 5,
		FLAG_IGNORE = 1 << 7,
		FLAG_TMP_DETACHED = 1 << 8,
		FLAG_ADAPTER_POSITION_UNKNOWN = 1 << 9,
		FLAG_ADAPTER_FULLUPDATE = 1 << 10,
		FLAG_MOVED = 1 << 11,
		FLAG_APPEARED_IN_PRE_LAYOUT = 1 << 12,
		FLAG_BOUNCED_FROM_HIDDEN_LIST = 1 << 13,
	};

	ViewHolder();
	explicit ViewHolder(Control *p_control);
	~ViewHolder() override;

	void set_control(Control *p_control);
	Control *get_control() const;

	int get_item_view_type() const { return m_item_view_type; }
	void set_item_view_type(int p_type) { m_item_view_type = p_type; }

	int64_t get_stable_id() const { return m_item_id; }
	void set_stable_id(int64_t p_id) { m_item_id = p_id; }

	int get_layout_position() const { return m_pre_layout_position == NO_POSITION ? m_position : m_pre_layout_position; }
	int get_position() const { return m_position; }
	void set_position(int p_position) { m_position = p_position; }
	int get_old_position() const { return m_old_position; }

	bool is_bound() const { return (m_flags & FLAG_BOUND) != 0; }
	bool is_updated() const { return (m_flags & FLAG_UPDATE) != 0; }
	bool is_invalid() const { return (m_flags & FLAG_INVALID) != 0; }
	bool is_removed() const { return (m_flags & FLAG_REMOVED) != 0; }
	bool should_ignore() const { return (m_flags & FLAG_IGNORE) != 0; }
	bool is_tmp_detached() const { return (m_flags & FLAG_TMP_DETACHED) != 0; }
	bool is_adapter_position_unknown() const { return (m_flags & FLAG_ADAPTER_POSITION_UNKNOWN) != 0; }

	// True once the control has been mounted at least once. The control's scene
	// ran its ready pass (@onready references populated) on that first mount;
	// an unmounted control's @onready refs are still null, so binding it
	// directly would run _bind_item against an unready scene. Survives
	// reset_internal (a recycled holder keeps its ready state).
	bool has_mounted_once() const { return m_mounted_once; }
	void mark_mounted_once() { m_mounted_once = true; }

	bool is_recyclable() const { return (m_flags & FLAG_NOT_RECYCLABLE) == 0 && m_is_recyclable_count <= 0; }
	void set_is_recyclable(bool p_recyclable);

	void flag_removed_and_offset_position(int p_new_position, int p_offset, bool p_apply_to_pre_layout);
	void offset_position(int p_offset, bool p_apply_to_pre_layout);
	void clear_old_position();
	void save_old_position();

	void set_flags(int p_flags, int p_mask);
	void add_flags(int p_flags);
	void clear_payload();
	void reset_internal();

	int get_flags() const { return m_flags; }

private:
	Control *m_control = nullptr;
	int m_position = NO_POSITION;
	int m_old_position = NO_POSITION;
	int64_t m_item_id = NO_ID;
	int m_item_view_type = INVALID_TYPE;
	int m_pre_layout_position = NO_POSITION;
	int m_flags = 0;
	int m_is_recyclable_count = 0;
	bool m_mounted_once = false;
};

} // namespace godot
