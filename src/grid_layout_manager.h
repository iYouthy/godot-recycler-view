#pragma once

#include "linear_layout_manager.h"
#include "span_size_lookup.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Port of GridLayoutManager. Arranges items in a grid of span_count columns
// (rows for horizontal grids), starting a new row when the accumulated span
// would exceed span_count. Each row takes the height of its tallest item and
// every item in it is stretched to that height. Item span sizes come from a
// SpanSizeLookup (default: one span each). Reuses the LinearLayoutManager
// on_layout_children skeleton through the layout-model hooks.
class GridLayoutManager : public LinearLayoutManager {
	GDCLASS(GridLayoutManager, LinearLayoutManager)

protected:
	static void _bind_methods();

	// Layout-model hooks (grid row model).
	void build_layout(RecyclerView *p_recycler_view, int p_item_count) const override;
	int content_size() const override;
	int first_visible_position(int p_scroll_offset, int p_item_count) const override;
	int last_visible_position(int p_scroll_end, int p_item_count) const override;
	void position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const override;

	int m_span_count = 2;
	Ref<SpanSizeLookup> m_span_size_lookup;
	mutable Vector<int> m_row_of_position;
	mutable Vector<int> m_column_of_position;
	mutable Vector<int> m_span_of_position;
	mutable Vector<int> m_row_offset;
	mutable Vector<int> m_row_height;
	mutable Vector<int> m_cell_borders;
	mutable int m_row_count = 0;
	mutable int m_total_content = 0;
	mutable bool m_rows_dirty = true;
	mutable int m_cached_item_count = -1;
	mutable int m_cached_span_count = -1;

public:
	void set_span_count(int p_span_count);
	int get_span_count() const { return m_span_count; }

	void set_span_size_lookup(const Ref<SpanSizeLookup> &p_lookup);
	Ref<SpanSizeLookup> get_span_size_lookup() const { return m_span_size_lookup; }

	// Row model introspection (tests / generic API).
	int get_item_row(int p_position) const;
	int get_item_column(int p_position) const;
	int get_row_offset(int p_row) const;
	int get_row_height(int p_row) const;
	int get_position_offset(int p_position) const override { return get_row_offset(get_item_row(p_position)); }
	int get_row_count() const { return m_row_count; }
	int get_cached_item_count() const override { return m_cached_item_count; }
	Rect2 get_item_rect(RecyclerView *p_recycler_view, int p_position) const override;

	void on_data_changed() override;

private:
	int get_span_size(RecyclerView *p_recycler_view, int p_position) const;
};

} // namespace godot
