#include "grid_layout_manager.h"

#include "layout_math.h"
#include "recycler_view.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

void GridLayoutManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_span_count", "span_count"), &GridLayoutManager::set_span_count);
	ClassDB::bind_method(D_METHOD("get_span_count"), &GridLayoutManager::get_span_count);
	ClassDB::bind_method(D_METHOD("set_span_size_lookup", "lookup"), &GridLayoutManager::set_span_size_lookup);
	ClassDB::bind_method(D_METHOD("get_span_size_lookup"), &GridLayoutManager::get_span_size_lookup);
	ClassDB::bind_method(D_METHOD("get_item_row", "position"), &GridLayoutManager::get_item_row);
	ClassDB::bind_method(D_METHOD("get_item_column", "position"), &GridLayoutManager::get_item_column);
	ClassDB::bind_method(D_METHOD("get_row_offset", "row"), &GridLayoutManager::get_row_offset);
	ClassDB::bind_method(D_METHOD("get_row_height", "row"), &GridLayoutManager::get_row_height);
	ClassDB::bind_method(D_METHOD("get_row_count"), &GridLayoutManager::get_row_count);
}

void GridLayoutManager::set_span_count(int p_span_count) {
	if (p_span_count < 1) {
		ERR_PRINT("GridLayoutManager span count should be at least 1.");
		return;
	}
	if (p_span_count == m_span_count) {
		return;
	}
	m_span_count = p_span_count;
	on_data_changed();
}

void GridLayoutManager::set_span_size_lookup(const Ref<SpanSizeLookup> &p_lookup) {
	m_span_size_lookup = p_lookup;
	on_data_changed();
}

void GridLayoutManager::on_data_changed() {
	m_rows_dirty = true;
}

int GridLayoutManager::get_span_size(RecyclerView *p_recycler_view, int p_position) const {
	int span = m_span_size_lookup.is_valid() ? m_span_size_lookup->get_span_size(p_position) : 1;
	return CLAMP(span, 1, m_span_count);
}

void GridLayoutManager::build_layout(RecyclerView *p_recycler_view, int p_item_count) const {
	// Cell borders depend on the current cross-axis viewport; refresh every call.
	const int cross_size = m_orientation == VERTICAL
			? (int)p_recycler_view->get_viewport_size().x
			: (int)p_recycler_view->get_viewport_size().y;
	m_cell_borders = calculate_item_borders(m_span_count, cross_size);

	if (!m_rows_dirty && m_cached_item_count == p_item_count && m_cached_span_count == m_span_count) {
		return;
	}

	m_row_of_position.resize(p_item_count);
	m_column_of_position.resize(p_item_count);
	m_span_of_position.resize(p_item_count);
	m_row_offset.clear();
	m_row_height.clear();

	int row = 0;
	int col_used = 0;
	int row_height = 0;
	int total_offset = 0;
	for (int pos = 0; pos < p_item_count; pos++) {
		const int span = get_span_size(p_recycler_view, pos);
		if (col_used + span > m_span_count) {
			m_row_offset.push_back(total_offset);
			m_row_height.push_back(row_height);
			total_offset += row_height;
			row_height = 0;
			col_used = 0;
			row++;
		}
		m_row_of_position.write[pos] = row;
		m_column_of_position.write[pos] = col_used;
		m_span_of_position.write[pos] = span;
		row_height = MAX(row_height, p_recycler_view->get_item_extent(pos));
		col_used += span;
	}
	m_row_offset.push_back(total_offset);
	m_row_height.push_back(row_height);
	m_total_content = total_offset + row_height;
	m_row_count = row + 1;
	m_rows_dirty = false;
	m_cached_item_count = p_item_count;
	m_cached_span_count = m_span_count;
}

int GridLayoutManager::content_size() const {
	return m_total_content;
}

int GridLayoutManager::first_visible_position(int p_scroll_offset, int p_item_count) const {
	// First row whose next row offset exceeds the scroll offset.
	const int upper = upper_bound(m_row_offset, m_row_count, p_scroll_offset);
	const int first_row = CLAMP(upper - 1, 0, m_row_count - 1);
	return lower_bound(m_row_of_position, p_item_count, first_row);
}

int GridLayoutManager::last_visible_position(int p_scroll_end, int p_item_count) const {
	// First row that starts at or beyond the viewport's bottom edge.
	const int last_row = MIN(lower_bound(m_row_offset, m_row_count, p_scroll_end), m_row_count);
	return lower_bound(m_row_of_position, p_item_count, last_row);
}

void GridLayoutManager::position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const {
	const int row = m_row_of_position[p_position];
	const int col = m_column_of_position[p_position];
	const int span = m_span_of_position[p_position];
	int main_offset = m_row_offset[row] - p_scroll_offset;
	const int main_length = m_row_height[row];
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL
				? (int)p_recycler_view->get_viewport_size().y
				: (int)p_recycler_view->get_viewport_size().x;
		main_offset = viewport_main - (m_row_offset[row] + main_length) + p_scroll_offset;
	}
	const int cross_start = m_cell_borders[col];
	const int cross_size = m_cell_borders[col + span] - m_cell_borders[col];
	if (m_orientation == VERTICAL) {
		p_recycler_view->set_item_view_position(p_holder,
				Vector2((float)cross_start, (float)main_offset),
				Vector2((float)cross_size, (float)main_length));
	} else {
		p_recycler_view->set_item_view_position(p_holder,
				Vector2((float)main_offset, (float)cross_start),
				Vector2((float)main_length, (float)cross_size));
	}
}

int GridLayoutManager::get_item_row(int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return 0;
	}
	return m_row_of_position[p_position];
}

int GridLayoutManager::get_item_column(int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return 0;
	}
	return m_column_of_position[p_position];
}

int GridLayoutManager::get_row_offset(int p_row) const {
	if (p_row < 0 || p_row >= m_row_count) {
		return 0;
	}
	return m_row_offset[p_row];
}

int GridLayoutManager::get_row_height(int p_row) const {
	if (p_row < 0 || p_row >= m_row_count) {
		return 0;
	}
	return m_row_height[p_row];
}

Rect2 GridLayoutManager::get_item_rect(RecyclerView *p_recycler_view, int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return Rect2();
	}
	const int scroll = m_orientation == VERTICAL
			? p_recycler_view->get_scroll_offset()
			: p_recycler_view->get_scroll_offset_horizontal();
	const int row = m_row_of_position[p_position];
	const int col = m_column_of_position[p_position];
	const int span = m_span_of_position[p_position];
	int main_offset = m_row_offset[row] - scroll;
	const int main_length = m_row_height[row];
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL
				? (int)p_recycler_view->get_viewport_size().y
				: (int)p_recycler_view->get_viewport_size().x;
		main_offset = viewport_main - (m_row_offset[row] + main_length) + scroll;
	}
	const int cross_start = m_cell_borders[col];
	const int cross_size = m_cell_borders[col + span] - m_cell_borders[col];
	if (m_orientation == VERTICAL) {
		return Rect2((float)cross_start, (float)main_offset, (float)cross_size, (float)main_length);
	} else {
		return Rect2((float)main_offset, (float)cross_start, (float)main_length, (float)cross_size);
	}
}

} // namespace godot
