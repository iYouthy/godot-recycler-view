#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

class DiffResultData;

// Pure C++ interfaces used by the diff algorithm. Deliberately free of any
// Godot runtime dependency (no GDCLASS, no Variant, no String) so the algorithm
// can be compiled and unit-tested as a standalone binary.

class DiffCallback {
public:
	virtual ~DiffCallback() = default;

	virtual int get_old_list_size() = 0;
	virtual int get_new_list_size() = 0;
	virtual bool are_items_the_same(int p_old_item_position, int p_new_item_position) = 0;
	virtual bool are_contents_the_same(int p_old_item_position, int p_new_item_position) = 0;
	// Returns an opaque payload token; the value itself is never inspected by the algorithm.
	virtual const void *get_change_payload(int p_old_item_position, int p_new_item_position) = 0;
};

class DiffListUpdateCallback {
public:
	virtual ~DiffListUpdateCallback() = default;

	virtual void on_inserted(int p_position, int p_count) = 0;
	virtual void on_removed(int p_position, int p_count) = 0;
	virtual void on_moved(int p_from_position, int p_to_position) = 0;
	virtual void on_changed(int p_position, int p_count, const void *p_payload) = 0;
};

// Port of androidx.recyclerview.widget.DiffUtil (Myers' diff algorithm).
class DiffAlgorithm {
public:
	// A diagonal is a run of matched items. Ranges are [start, end).
	struct Diagonal {
		int x;
		int y;
		int size;

		Diagonal() :
				x(0), y(0), size(0) {}

		Diagonal(int p_x, int p_y, int p_size) :
				x(p_x), y(p_y), size(p_size) {}

		int end_x() const { return x + size; }
		int end_y() const { return y + size; }

		bool operator<(const Diagonal &p_other) const {
			if (x != p_other.x) {
				return x < p_other.x;
			}
			return y < p_other.y;
		}
	};

	static DiffResultData calculate_diff(DiffCallback &p_callback, bool p_detect_moves);
};

// Alias so DiffResultData and tests can refer to the algorithm's diagonal type.
using Diagonal = DiffAlgorithm::Diagonal;

// Result of a diff: item statuses + diagonals, with conversion and dispatch helpers.
class DiffResultData {
public:
	static constexpr int NO_POSITION = -1;

	// Runs the Myers pass; statuses are zero-filled, edge diagonals added and moves matched.
	DiffResultData(DiffCallback &p_callback, Vector<Diagonal> p_diagonals, Vector<int> p_old_item_statuses, Vector<int> p_new_item_statuses, bool p_detect_moves);

	int convert_old_position_to_new(int p_old_list_position) const;
	int convert_new_position_to_old(int p_new_list_position) const;
	void dispatch_updates_to(DiffListUpdateCallback &p_update_callback) const;

	int get_old_list_size() const { return m_old_list_size; }
	int get_new_list_size() const { return m_new_list_size; }

	// Exposed for the GDCLASS layer and tests.
	const Vector<DiffAlgorithm::Diagonal> &get_diagonals() const { return m_diagonals; }

private:
	enum Flag {
		FLAG_NOT_CHANGED = 1,
		FLAG_CHANGED = FLAG_NOT_CHANGED << 1,
		FLAG_MOVED_CHANGED = FLAG_CHANGED << 1,
		FLAG_MOVED_NOT_CHANGED = FLAG_MOVED_CHANGED << 1,
		FLAG_MOVED = FLAG_MOVED_CHANGED | FLAG_MOVED_NOT_CHANGED,
		FLAG_OFFSET = 4,
		FLAG_MASK = (1 << FLAG_OFFSET) - 1,
	};

	void add_edge_diagonals();
	void find_matching_items();
	void find_move_matches();
	void find_matching_addition(int p_pos_x);

	Vector<DiffAlgorithm::Diagonal> m_diagonals;
	Vector<int> m_old_item_statuses;
	Vector<int> m_new_item_statuses;
	DiffCallback *m_callback;
	int m_old_list_size;
	int m_new_list_size;
	bool m_detect_moves;
};

} // namespace godot
