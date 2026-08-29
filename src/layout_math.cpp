#include "layout_math.h"

namespace godot {

int upper_bound(const Vector<int> &p_offsets, int p_size, int p_value) {
	int lo = 0;
	int hi = p_size;
	while (lo < hi) {
		const int mid = (lo + hi) / 2;
		if (p_offsets[mid] <= p_value) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}
	return lo;
}

int lower_bound(const Vector<int> &p_offsets, int p_size, int p_value) {
	int lo = 0;
	int hi = p_size;
	while (lo < hi) {
		const int mid = (lo + hi) / 2;
		if (p_offsets[mid] < p_value) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}
	return lo;
}

Vector<int> calculate_item_borders(int p_span_count, int p_total_space) {
	Vector<int> borders;
	borders.resize(p_span_count + 1);
	borders.write[0] = 0;
	const int size_per_span = p_total_space / p_span_count;
	const int remainder = p_total_space % p_span_count;
	int consumed_pixels = 0;
	int additional_size = 0;
	for (int i = 1; i <= p_span_count; i++) {
		int item_extent = size_per_span;
		additional_size += remainder;
		if (additional_size > 0 && (p_span_count - additional_size) < remainder) {
			item_extent += 1;
			additional_size -= p_span_count;
		}
		consumed_pixels += item_extent;
		borders.write[i] = consumed_pixels;
	}
	return borders;
}

} // namespace godot
