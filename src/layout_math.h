#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Pure helpers shared by the layout managers (doctest-testable).

// First index in [0, p_size) with p_offsets[idx] > p_value; p_size if none.
int upper_bound(const Vector<int> &p_offsets, int p_size, int p_value);

// First index in [0, p_size) with p_offsets[idx] >= p_value; p_size if none.
int lower_bound(const Vector<int> &p_offsets, int p_size, int p_value);

// Grid cell boundaries: evenly distributes total_space across span_count cells,
// handing the remainder pixels to cells along the way (port of
// GridLayoutManager.calculateItemBorders). Caller must ensure span_count >= 1.
Vector<int> calculate_item_borders(int p_span_count, int p_total_space);

} // namespace godot
