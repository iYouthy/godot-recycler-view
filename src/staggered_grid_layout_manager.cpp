#include "staggered_grid_layout_manager.h"

#include "layout_math.h"
#include "recycler_view.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

#include <limits>

namespace godot {

void StaggeredGridLayoutManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_span_count", "span_count"), &StaggeredGridLayoutManager::set_span_count);
	ClassDB::bind_method(D_METHOD("get_span_count"), &StaggeredGridLayoutManager::get_span_count);
	ClassDB::bind_method(D_METHOD("get_item_column", "position"), &StaggeredGridLayoutManager::get_item_column);
	ClassDB::bind_method(D_METHOD("get_col_top_of_position", "position"), &StaggeredGridLayoutManager::get_col_top_of_position);
}

void StaggeredGridLayoutManager::set_span_count(int p_span_count) {
	if (p_span_count < 1) {
		ERR_PRINT("StaggeredGridLayoutManager span count should be at least 1.");
		return;
	}
	if (p_span_count == m_span_count) {
		return;
	}
	m_span_count = p_span_count;
	on_data_changed();
}

void StaggeredGridLayoutManager::on_data_changed() {
	m_layout_dirty = true;
}

void StaggeredGridLayoutManager::build_layout(RecyclerView *p_recycler_view, int p_item_count) const {
	// Column widths depend on the cross-axis viewport; refresh every call.
	const int cross_size = m_orientation == VERTICAL
			? (int)p_recycler_view->get_viewport_size().x
			: (int)p_recycler_view->get_viewport_size().y;
	m_cell_borders = calculate_item_borders(m_span_count, cross_size);

	if (!m_layout_dirty && m_cached_item_count == p_item_count && m_cached_span_count == m_span_count) {
		return;
	}

	m_column_of_position.resize(p_item_count);
	m_col_top_of_position.resize(p_item_count);
	m_column_positions.resize(m_span_count);
	m_column_tops.resize(m_span_count);
	m_column_ends.resize(m_span_count);
	for (int c = 0; c < m_span_count; c++) {
		m_column_positions.write[c].clear();
		m_column_tops.write[c].clear();
		m_column_ends.write[c].clear();
	}

	Vector<int> col_ends;
	col_ends.resize(m_span_count);
	for (int c = 0; c < m_span_count; c++) {
		col_ends.write[c] = 0;
	}

	// Each item flows into the currently shortest column (first wins ties, like
	// Android's getNextSpan). Columns accumulate heights independently.
	for (int pos = 0; pos < p_item_count; pos++) {
		int col = 0;
		int min_end = col_ends[0];
		for (int c = 1; c < m_span_count; c++) {
			if (col_ends[c] < min_end) {
				min_end = col_ends[c];
				col = c;
			}
		}
		const int top = col_ends[col];
		const int end = top + p_recycler_view->get_item_height(pos);
		col_ends.write[col] = end;

		m_column_of_position.write[pos] = col;
		m_col_top_of_position.write[pos] = top;
		m_column_positions.write[col].push_back(pos);
		m_column_tops.write[col].push_back(top);
		m_column_ends.write[col].push_back(end);
	}

	m_content_size = 0;
	for (int c = 0; c < m_span_count; c++) {
		if (col_ends[c] > m_content_size) {
			m_content_size = col_ends[c];
		}
	}
	m_layout_dirty = false;
	m_cached_item_count = p_item_count;
	m_cached_span_count = m_span_count;
}

int StaggeredGridLayoutManager::content_size() const {
	return m_content_size;
}

int StaggeredGridLayoutManager::first_visible_position(int p_scroll_offset, int p_item_count) const {
	// Smallest position, across all columns, whose item bottom crosses the
	// viewport top (each column's ends are monotonic).
	int best = std::numeric_limits<int>::max();
	for (int c = 0; c < m_span_count; c++) {
		const Vector<int> &ends = m_column_ends[c];
		const int idx = upper_bound(ends, ends.size(), p_scroll_offset);
		if (idx < ends.size()) {
			const int pos = m_column_positions[c][idx];
			if (pos < best) {
				best = pos;
			}
		}
	}
	return best == std::numeric_limits<int>::max() ? 0 : best;
}

int StaggeredGridLayoutManager::last_visible_position(int p_scroll_end, int p_item_count) const {
	// Largest position, across all columns, whose item top is still above the
	// viewport bottom; returns the exclusive bound (first non-visible position),
	// like LinearLayoutManager.
	int best = std::numeric_limits<int>::min();
	for (int c = 0; c < m_span_count; c++) {
		const Vector<int> &tops = m_column_tops[c];
		const int idx = lower_bound(tops, tops.size(), p_scroll_end);
		if (idx > 0) {
			const int pos = m_column_positions[c][idx - 1];
			if (pos > best) {
				best = pos;
			}
		}
	}
	if (best == std::numeric_limits<int>::min()) {
		return 0;
	}
	return MIN(best + 1, p_item_count);
}

void StaggeredGridLayoutManager::position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const {
	const int col = m_column_of_position[p_position];
	const int top = m_col_top_of_position[p_position];
	int main_offset = top - p_scroll_offset;
	const int main_length = p_recycler_view->get_item_height(p_position);
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL
				? (int)p_recycler_view->get_viewport_size().y
				: (int)p_recycler_view->get_viewport_size().x;
		main_offset = viewport_main - (top + main_length) + p_scroll_offset;
	}
	const int cross_start = m_cell_borders[col];
	const int cross_size = m_cell_borders[col + 1] - m_cell_borders[col];
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

int StaggeredGridLayoutManager::get_item_column(int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return 0;
	}
	return m_column_of_position[p_position];
}

int StaggeredGridLayoutManager::get_col_top_of_position(int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return 0;
	}
	return m_col_top_of_position[p_position];
}

int StaggeredGridLayoutManager::get_position_offset(int p_position) const {
	return get_col_top_of_position(p_position);
}

Rect2 StaggeredGridLayoutManager::get_item_rect(RecyclerView *p_recycler_view, int p_position) const {
	if (p_position < 0 || p_position >= m_cached_item_count) {
		return Rect2();
	}
	const int scroll = m_orientation == VERTICAL
			? p_recycler_view->get_scroll_offset()
			: p_recycler_view->get_scroll_offset_horizontal();
	const int col = m_column_of_position[p_position];
	const int top = m_col_top_of_position[p_position];
	int main_offset = top - scroll;
	const int main_length = p_recycler_view->get_item_height(p_position);
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL
				? (int)p_recycler_view->get_viewport_size().y
				: (int)p_recycler_view->get_viewport_size().x;
		main_offset = viewport_main - (top + main_length) + scroll;
	}
	const int cross_start = m_cell_borders[col];
	const int cross_size = m_cell_borders[col + 1] - m_cell_borders[col];
	if (m_orientation == VERTICAL) {
		return Rect2((float)cross_start, (float)main_offset, (float)cross_size, (float)main_length);
	}
	return Rect2((float)main_offset, (float)cross_start, (float)main_length, (float)cross_size);
}

} // namespace godot
