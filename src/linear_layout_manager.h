#pragma once

#include "layout_manager.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "view_holder.h"

namespace godot {

// Port of LinearLayoutManager. Lays items out in a single row/column, supporting
// variable item heights. A cumulative offset table maps each position to its
// start coordinate; virtualization instantiates only the items intersecting the
// viewport. The layout-model hooks (build_layout / content_size /
// first/last_visible_position / position_holder) are overridable by subclasses
// such as GridLayoutManager, which reuses the on_layout_children skeleton.
class LinearLayoutManager : public LayoutManager {
	GDCLASS(LinearLayoutManager, LayoutManager)

protected:
	static void _bind_methods();

	// Layout-model hooks.
	virtual void build_layout(RecyclerView *p_recycler_view, int p_item_count) const;
	virtual int content_size() const;
	virtual int first_visible_position(int p_scroll_offset, int p_item_count) const;
	virtual int last_visible_position(int p_scroll_end, int p_item_count) const;
	virtual void position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const;

	int m_orientation = VERTICAL;
	mutable Vector<int> m_offsets;
	mutable bool m_offsets_dirty = true;
	mutable int m_cached_item_count = -1;

public:
	enum Orientation {
		VERTICAL = 0,
		HORIZONTAL = 1,
	};

	void set_orientation(int p_orientation);
	int get_orientation() const { return m_orientation; }

	void on_layout_children(RecyclerView *p_recycler_view, State *p_state) override;
	bool can_scroll_vertically() const override { return m_orientation == VERTICAL; }
	bool can_scroll_horizontally() const override { return m_orientation == HORIZONTAL; }
	int get_content_size(RecyclerView *p_recycler_view) const override;
	void on_data_changed() override;

	// Start coordinate of the item at the given position (offsets table).
	int get_item_offset(int p_position) const;
	virtual int get_cached_item_count() const { return m_offsets.size() - 1; }
	Rect2 get_item_rect(RecyclerView *p_recycler_view, int p_position) const override;
	void collect_adjacent_prefetch_positions(int p_dy, RecyclerView *p_recycler_view, Array &r_positions) const override;

private:
	bool has_child_at(RecyclerView *p_recycler_view, int p_position) const;
};

} // namespace godot
