#include "diff_algo.h"

namespace godot {

// ---------------------------------------------------------------------------
// Internal helper structures (port of DiffUtil.java internals).

namespace {

// A match segment found by Myers' algorithm, optionally prefixed/postfixed with
// an add or remove operation.
struct Snake {
	int start_x = 0;
	int start_y = 0;
	int end_x = 0;
	int end_y = 0;
	bool reverse = false;

	bool has_addition_or_removal() const { return end_y - start_y != end_x - start_x; }
	bool is_addition() const { return end_y - start_y > end_x - start_x; }
	int diagonal_size() const { return (end_x - start_x) < (end_y - start_y) ? (end_x - start_x) : (end_y - start_y); }

	DiffAlgorithm::Diagonal to_diagonal() const {
		if (has_addition_or_removal()) {
			if (reverse) {
				return DiffAlgorithm::Diagonal(start_x, start_y, diagonal_size());
			}
			if (is_addition()) {
				return DiffAlgorithm::Diagonal(start_x, start_y + 1, diagonal_size());
			}
			return DiffAlgorithm::Diagonal(start_x + 1, start_y, diagonal_size());
		}
		return DiffAlgorithm::Diagonal(start_x, start_y, end_x - start_x);
	}
};

// A range in both lists that still needs solving. Ends are exclusive.
struct Range {
	int old_list_start = 0;
	int old_list_end = 0;
	int new_list_start = 0;
	int new_list_end = 0;

	Range() {}

	Range(int p_old_start, int p_old_end, int p_new_start, int p_new_end) :
			old_list_start(p_old_start), old_list_end(p_old_end), new_list_start(p_new_start), new_list_end(p_new_end) {}

	int old_size() const { return old_list_end - old_list_start; }
	int new_size() const { return new_list_end - new_list_start; }
};

// Array with negative-index support for the Myers' k-lines.
class CenteredArray {
	Vector<int> m_data;
	int m_mid = 0;

public:
	CenteredArray(int p_size) {
		m_data.resize(p_size);
		m_mid = p_size / 2;
	}

	int get(int p_index) const { return m_data[p_index + m_mid]; }
	void set(int p_index, int p_value) { m_data.write[p_index + m_mid] = p_value; }
	const Vector<int> &backing_data() const { return m_data; }
};

// An update skipped because it was a move; tracked until its counterpart is found.
struct PostponedUpdate {
	int pos_in_owner_list;
	int current_pos;
	bool removal;

	PostponedUpdate() :
			pos_in_owner_list(0), current_pos(0), removal(false) {}

	PostponedUpdate(int p_pos_in_owner_list, int p_current_pos, bool p_removal) :
			pos_in_owner_list(p_pos_in_owner_list), current_pos(p_current_pos), removal(p_removal) {}
};

bool get_postponed_update(Vector<PostponedUpdate> &p_postponed_updates, int p_pos_in_list, bool p_removal, PostponedUpdate &r_result) {
	for (int i = 0; i < p_postponed_updates.size(); i++) {
		if (p_postponed_updates[i].pos_in_owner_list == p_pos_in_list && p_postponed_updates[i].removal == p_removal) {
			r_result = p_postponed_updates[i];
			p_postponed_updates.remove_at(i);
			// Re-offset the remaining updates that come after the matched one.
			for (int j = i; j < p_postponed_updates.size(); j++) {
				if (p_removal) {
					p_postponed_updates.write[j].current_pos--;
				} else {
					p_postponed_updates.write[j].current_pos++;
				}
			}
			return true;
		}
	}
	return false;
}

int abs_i(int p_value) { return p_value < 0 ? -p_value : p_value; }

bool forward_search(const Range &p_range, DiffCallback &p_cb, CenteredArray &p_forward, CenteredArray &p_backward, int p_d, Snake &r_snake);
bool backward_search(const Range &p_range, DiffCallback &p_cb, CenteredArray &p_forward, CenteredArray &p_backward, int p_d, Snake &r_snake);

// ---------------------------------------------------------------------------
// Myers' divide-and-conquer search.

bool mid_point(const Range &p_range, DiffCallback &p_cb, CenteredArray &p_forward, CenteredArray &p_backward, Snake &r_snake) {
	if (p_range.old_size() < 1 || p_range.new_size() < 1) {
		return false;
	}
	int max = (p_range.old_size() + p_range.new_size() + 1) / 2;
	p_forward.set(1, p_range.old_list_start);
	p_backward.set(1, p_range.old_list_end);
	for (int d = 0; d < max; d++) {
		if (forward_search(p_range, p_cb, p_forward, p_backward, d, r_snake)) {
			return true;
		}
		if (backward_search(p_range, p_cb, p_forward, p_backward, d, r_snake)) {
			return true;
		}
	}
	return false;
}

bool forward_search(const Range &p_range, DiffCallback &p_cb, CenteredArray &p_forward, CenteredArray &p_backward, int p_d, Snake &r_snake) {
	int delta = p_range.old_size() - p_range.new_size();
	bool check_for_snake = abs_i(delta) % 2 == 1;
	for (int k = -p_d; k <= p_d; k += 2) {
		int x;
		int start_x;
		if (k == -p_d || (k != p_d && p_forward.get(k + 1) > p_forward.get(k - 1))) {
			x = start_x = p_forward.get(k + 1);
		} else {
			start_x = p_forward.get(k - 1);
			x = start_x + 1;
		}
		int y = p_range.new_list_start + (x - p_range.old_list_start) - k;
		int start_y = (p_d == 0 || x != start_x) ? y : y - 1;
		while (x < p_range.old_list_end && y < p_range.new_list_end && p_cb.are_items_the_same(x, y)) {
			x++;
			y++;
		}
		p_forward.set(k, x);
		if (check_for_snake) {
			int backwards_k = delta - k;
			if (backwards_k >= -p_d + 1 && backwards_k <= p_d - 1 && p_backward.get(backwards_k) <= x) {
				r_snake.start_x = start_x;
				r_snake.start_y = start_y;
				r_snake.end_x = x;
				r_snake.end_y = y;
				r_snake.reverse = false;
				return true;
			}
		}
	}
	return false;
}

bool backward_search(const Range &p_range, DiffCallback &p_cb, CenteredArray &p_forward, CenteredArray &p_backward, int p_d, Snake &r_snake) {
	int delta = p_range.old_size() - p_range.new_size();
	bool check_for_snake = abs_i(delta) % 2 == 0;
	for (int k = -p_d; k <= p_d; k += 2) {
		int x;
		int start_x;
		if (k == -p_d || (k != p_d && p_backward.get(k + 1) < p_backward.get(k - 1))) {
			x = start_x = p_backward.get(k + 1);
		} else {
			start_x = p_backward.get(k - 1);
			x = start_x - 1;
		}
		int y = p_range.new_list_end - ((p_range.old_list_end - x) - k);
		int start_y = (p_d == 0 || x != start_x) ? y : y + 1;
		while (x > p_range.old_list_start && y > p_range.new_list_start && p_cb.are_items_the_same(x - 1, y - 1)) {
			x--;
			y--;
		}
		p_backward.set(k, x);
		if (check_for_snake) {
			int forwards_k = delta - k;
			if (forwards_k >= -p_d && forwards_k <= p_d && p_forward.get(forwards_k) >= x) {
				r_snake.start_x = x;
				r_snake.start_y = y;
				r_snake.end_x = start_x;
				r_snake.end_y = start_y;
				r_snake.reverse = true;
				return true;
			}
		}
	}
	return false;
}

} // namespace

DiffResultData DiffAlgorithm::calculate_diff(DiffCallback &p_callback, bool p_detect_moves) {
	const int old_size = p_callback.get_old_list_size();
	const int new_size = p_callback.get_new_list_size();

	Vector<Diagonal> diagonals;
	Vector<Range> stack;
	stack.push_back(Range(0, old_size, 0, new_size));

	const int max = (old_size + new_size + 1) / 2;
	CenteredArray forward(max * 2 + 1);
	CenteredArray backward(max * 2 + 1);

	Vector<Range> range_pool;
	while (!stack.is_empty()) {
		Range range = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);

		Snake snake;
		if (mid_point(range, p_callback, forward, backward, snake)) {
			if (snake.diagonal_size() > 0) {
				diagonals.push_back(snake.to_diagonal());
			}
			Range left;
			if (!range_pool.is_empty()) {
				left = range_pool[range_pool.size() - 1];
				range_pool.remove_at(range_pool.size() - 1);
			}
			left.old_list_start = range.old_list_start;
			left.new_list_start = range.new_list_start;
			left.old_list_end = snake.start_x;
			left.new_list_end = snake.start_y;
			stack.push_back(left);

			Range right = range;
			right.old_list_end = range.old_list_end;
			right.new_list_end = range.new_list_end;
			right.old_list_start = snake.end_x;
			right.new_list_start = snake.end_y;
			stack.push_back(right);
		} else {
			range_pool.push_back(range);
		}
	}

	diagonals.sort();
	return DiffResultData(p_callback, diagonals, forward.backing_data(), backward.backing_data(), p_detect_moves);
}

// ---------------------------------------------------------------------------
// DiffResultData.

DiffResultData::DiffResultData(DiffCallback &p_callback, Vector<Diagonal> p_diagonals, Vector<int> p_old_item_statuses, Vector<int> p_new_item_statuses, bool p_detect_moves) :
		m_diagonals(p_diagonals), m_old_item_statuses(p_old_item_statuses), m_new_item_statuses(p_new_item_statuses), m_callback(&p_callback), m_old_list_size(p_callback.get_old_list_size()), m_new_list_size(p_callback.get_new_list_size()), m_detect_moves(p_detect_moves) {
	m_old_item_statuses.fill(0);
	m_new_item_statuses.fill(0);
	add_edge_diagonals();
	find_matching_items();
}

void DiffResultData::add_edge_diagonals() {
	Diagonal first = m_diagonals.is_empty() ? Diagonal(0, 0, 0) : m_diagonals[0];
	if (m_diagonals.is_empty() || first.x != 0 || first.y != 0) {
		m_diagonals.insert(0, Diagonal(0, 0, 0));
	}
	m_diagonals.push_back(Diagonal(m_old_list_size, m_new_list_size, 0));
}

void DiffResultData::find_matching_items() {
	for (int di = 0; di < m_diagonals.size(); di++) {
		const Diagonal &diagonal = m_diagonals[di];
		for (int offset = 0; offset < diagonal.size; offset++) {
			int pos_x = diagonal.x + offset;
			int pos_y = diagonal.y + offset;
			bool the_same = m_callback->are_contents_the_same(pos_x, pos_y);
			int change_flag = the_same ? FLAG_NOT_CHANGED : FLAG_CHANGED;
			m_old_item_statuses.write[pos_x] = (pos_y << FLAG_OFFSET) | change_flag;
			m_new_item_statuses.write[pos_y] = (pos_x << FLAG_OFFSET) | change_flag;
		}
	}
	if (m_detect_moves) {
		find_move_matches();
	}
}

void DiffResultData::find_move_matches() {
	int pos_x = 0;
	for (int di = 0; di < m_diagonals.size(); di++) {
		const Diagonal &diagonal = m_diagonals[di];
		while (pos_x < diagonal.x) {
			if (m_old_item_statuses[pos_x] == 0) {
				find_matching_addition(pos_x);
			}
			pos_x++;
		}
		pos_x = diagonal.end_x();
	}
}

void DiffResultData::find_matching_addition(int p_pos_x) {
	int pos_y = 0;
	for (int i = 0; i < m_diagonals.size(); i++) {
		const Diagonal &diagonal = m_diagonals[i];
		while (pos_y < diagonal.y) {
			if (m_new_item_statuses[pos_y] == 0) {
				if (m_callback->are_items_the_same(p_pos_x, pos_y)) {
					bool contents_matching = m_callback->are_contents_the_same(p_pos_x, pos_y);
					int change_flag = contents_matching ? FLAG_MOVED_NOT_CHANGED : FLAG_MOVED_CHANGED;
					m_old_item_statuses.write[p_pos_x] = (pos_y << FLAG_OFFSET) | change_flag;
					m_new_item_statuses.write[pos_y] = (p_pos_x << FLAG_OFFSET) | change_flag;
					return;
				}
			}
			pos_y++;
		}
		pos_y = diagonal.end_y();
	}
}

int DiffResultData::convert_old_position_to_new(int p_old_list_position) const {
	if (p_old_list_position < 0 || p_old_list_position >= m_old_list_size) {
		return NO_POSITION;
	}
	const int status = m_old_item_statuses[p_old_list_position];
	if ((status & FLAG_MASK) == 0) {
		return NO_POSITION;
	}
	return status >> FLAG_OFFSET;
}

int DiffResultData::convert_new_position_to_old(int p_new_list_position) const {
	if (p_new_list_position < 0 || p_new_list_position >= m_new_list_size) {
		return NO_POSITION;
	}
	const int status = m_new_item_statuses[p_new_list_position];
	if ((status & FLAG_MASK) == 0) {
		return NO_POSITION;
	}
	return status >> FLAG_OFFSET;
}

void DiffResultData::dispatch_updates_to(DiffListUpdateCallback &p_update_callback) const {
	int current_list_size = m_old_list_size;
	Vector<PostponedUpdate> postponed_updates;
	int pos_x = m_old_list_size;
	int pos_y = m_new_list_size;
	for (int diagonal_index = m_diagonals.size() - 1; diagonal_index >= 0; diagonal_index--) {
		const Diagonal &diagonal = m_diagonals[diagonal_index];
		int end_x = diagonal.end_x();
		int end_y = diagonal.end_y();
		while (pos_x > end_x) {
			pos_x--;
			const int status = m_old_item_statuses[pos_x];
			if ((status & FLAG_MOVED) != 0) {
				const int new_pos = status >> FLAG_OFFSET;
				PostponedUpdate postponed_update;
				if (get_postponed_update(postponed_updates, new_pos, false, postponed_update)) {
					const int updated_new_pos = current_list_size - postponed_update.current_pos;
					p_update_callback.on_moved(pos_x, updated_new_pos - 1);
					if ((status & FLAG_MOVED_CHANGED) != 0) {
						const void *change_payload = m_callback->get_change_payload(pos_x, new_pos);
						p_update_callback.on_changed(updated_new_pos - 1, 1, change_payload);
					}
				} else {
					postponed_updates.push_back(PostponedUpdate(pos_x, current_list_size - pos_x - 1, true));
				}
			} else {
				p_update_callback.on_removed(pos_x, 1);
				current_list_size--;
			}
		}
		while (pos_y > end_y) {
			pos_y--;
			const int status = m_new_item_statuses[pos_y];
			if ((status & FLAG_MOVED) != 0) {
				const int old_pos = status >> FLAG_OFFSET;
				PostponedUpdate postponed_update;
				if (!get_postponed_update(postponed_updates, old_pos, true, postponed_update)) {
					postponed_updates.push_back(PostponedUpdate(pos_y, current_list_size - pos_x, false));
				} else {
					const int updated_old_pos = current_list_size - postponed_update.current_pos - 1;
					p_update_callback.on_moved(updated_old_pos, pos_x);
					if ((status & FLAG_MOVED_CHANGED) != 0) {
						const void *change_payload = m_callback->get_change_payload(old_pos, pos_y);
						p_update_callback.on_changed(pos_x, 1, change_payload);
					}
				}
			} else {
				p_update_callback.on_inserted(pos_x, 1);
				current_list_size++;
			}
		}
		pos_x = diagonal.x;
		pos_y = diagonal.y;
		for (int i = 0; i < diagonal.size; i++) {
			if ((m_old_item_statuses[pos_x] & FLAG_MASK) == FLAG_CHANGED) {
				const void *change_payload = m_callback->get_change_payload(pos_x, pos_y);
				p_update_callback.on_changed(pos_x, 1, change_payload);
			}
			pos_x++;
			pos_y++;
		}
		pos_x = diagonal.x;
		pos_y = diagonal.y;
	}
}

} // namespace godot
