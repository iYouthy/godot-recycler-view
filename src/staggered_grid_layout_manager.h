#pragma once

#include "linear_layout_manager.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Port of StaggeredGridLayoutManager: a masonry layout where each new item flows
// into the currently shortest column, so columns accumulate heights
// independently and items are staggered rather than row-aligned. Position and
// offset are NOT monotonic (a later item can sit above an earlier one), so the
// visible range is resolved per column. Reuses LinearLayoutManager's
// on_layout_children virtualization skeleton through the layout-model hooks.
class StaggeredGridLayoutManager : public LinearLayoutManager {
	GDCLASS(StaggeredGridLayoutManager, LinearLayoutManager)

protected:
	static void _bind_methods();

	// Layout-model hooks (staggered column model).
	void build_layout(RecyclerView *p_recycler_view, int p_item_count) const override;
	int content_size() const override;
	int first_visible_position(int p_scroll_offset, int p_item_count) const override;
	int last_visible_position(int p_scroll_end, int p_item_count) const override;
	void position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const override;

	int m_span_count = 2;
	// position -> column index / offset within its column.
	mutable Vector<int> m_column_of_position;
	mutable Vector<int> m_col_top_of_position;
	// Per-column item lists (offset order), with parallel top/end arrays for the
	// per-column binary searches in first/last_visible_position.
	mutable Vector<Vector<int>> m_column_positions;
	mutable Vector<Vector<int>> m_column_tops;
	mutable Vector<Vector<int>> m_column_ends;
	mutable Vector<int> m_cell_borders;
	mutable int m_content_size = 0;
	mutable int m_cached_item_count = -1;
	mutable int m_cached_span_count = -1;
	mutable bool m_layout_dirty = true;

public:
	void set_span_count(int p_span_count);
	int get_span_count() const { return m_span_count; }

	// Introspection (tests / generic API).
	int get_item_column(int p_position) const;
	int get_col_top_of_position(int p_position) const;
	int get_cached_item_count() const override { return m_cached_item_count; }
	Rect2 get_item_rect(RecyclerView *p_recycler_view, int p_position) const override;
	int get_position_offset(int p_position) const override;

	void on_data_changed() override;
};

} // namespace godot
