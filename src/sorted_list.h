#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.SortedList. Keeps items sorted using a
// callback's compare() and dispatches change notifications for each edit.
// Item values are stored by copy; the callback defines ordering and identity.

template <typename T>
class SortedListCoreCallback {
public:
	virtual ~SortedListCoreCallback() = default;

	virtual int compare(const T &p_o1, const T &p_o2) = 0;
	virtual bool are_items_the_same(const T &p_item1, const T &p_item2) = 0;
	virtual bool are_contents_the_same(const T &p_old_item, const T &p_new_item) = 0;
	virtual const void *get_change_payload(const T &p_item1, const T &p_item2) = 0;

	virtual void on_inserted(int p_position, int p_count) = 0;
	virtual void on_removed(int p_position, int p_count) = 0;
	virtual void on_moved(int p_from_position, int p_to_position) = 0;
	virtual void on_changed(int p_position, int p_count, const void *p_payload) = 0;
};

// Batches consecutive same-type notifications from the SortedList and forwards
// them to the wrapped callback, mirroring BatchingListUpdateCallback rules.
template <typename T>
class SortedListCoreBatchedCallback : public SortedListCoreCallback<T> {
	SortedListCoreCallback<T> *m_wrapped;
	int m_last_event_type = 0;
	int m_last_event_position = -1;
	int m_last_event_count = -1;
	const void *m_last_event_payload = nullptr;

	static const int TYPE_NONE = 0;
	static const int TYPE_ADD = 1;
	static const int TYPE_REMOVE = 2;
	static const int TYPE_CHANGE = 3;
	static const int TYPE_MOVE = 4;

public:
	SortedListCoreBatchedCallback() :
			m_wrapped(nullptr) {}

	explicit SortedListCoreBatchedCallback(SortedListCoreCallback<T> *p_wrapped) :
			m_wrapped(p_wrapped) {}

	SortedListCoreCallback<T> *get_wrapped_callback() const { return m_wrapped; }

	int compare(const T &p_o1, const T &p_o2) override { return m_wrapped->compare(p_o1, p_o2); }
	bool are_items_the_same(const T &p_item1, const T &p_item2) override { return m_wrapped->are_items_the_same(p_item1, p_item2); }
	bool are_contents_the_same(const T &p_old_item, const T &p_new_item) override { return m_wrapped->are_contents_the_same(p_old_item, p_new_item); }
	const void *get_change_payload(const T &p_item1, const T &p_item2) override { return m_wrapped->get_change_payload(p_item1, p_item2); }

	void on_inserted(int p_position, int p_count) override {
		if (m_last_event_type == TYPE_ADD && p_position >= m_last_event_position && p_position <= m_last_event_position + m_last_event_count) {
			m_last_event_count += p_count;
			m_last_event_position = p_position < m_last_event_position ? p_position : m_last_event_position;
			return;
		}
		dispatch_last_event();
		m_last_event_position = p_position;
		m_last_event_count = p_count;
		m_last_event_type = TYPE_ADD;
	}

	void on_removed(int p_position, int p_count) override {
		if (m_last_event_type == TYPE_REMOVE && m_last_event_position >= p_position && m_last_event_position <= p_position + p_count) {
			m_last_event_count += p_count;
			m_last_event_position = p_position;
			return;
		}
		dispatch_last_event();
		m_last_event_position = p_position;
		m_last_event_count = p_count;
		m_last_event_type = TYPE_REMOVE;
	}

	void on_moved(int p_from_position, int p_to_position) override {
		dispatch_last_event();
		m_last_event_position = p_from_position;
		m_last_event_count = p_to_position;
		m_last_event_type = TYPE_MOVE;
	}

	void on_changed(int p_position, int p_count, const void *p_payload) override {
		if (m_last_event_type == TYPE_CHANGE
				&& p_position <= m_last_event_position + m_last_event_count
				&& p_position + p_count >= m_last_event_position
				&& m_last_event_payload == p_payload) {
			m_last_event_position = p_position < m_last_event_position ? p_position : m_last_event_position;
			int end = p_position + p_count;
			m_last_event_count = end - m_last_event_position > m_last_event_count ? end - m_last_event_position : m_last_event_count;
			return;
		}
		dispatch_last_event();
		m_last_event_position = p_position;
		m_last_event_count = p_count;
		m_last_event_payload = p_payload;
		m_last_event_type = TYPE_CHANGE;
	}

	void dispatch_last_event() {
		if (m_last_event_type == TYPE_NONE) {
			return;
		}
		switch (m_last_event_type) {
			case TYPE_ADD:
				m_wrapped->on_inserted(m_last_event_position, m_last_event_count);
				break;
			case TYPE_REMOVE:
				m_wrapped->on_removed(m_last_event_position, m_last_event_count);
				break;
			case TYPE_CHANGE:
				m_wrapped->on_changed(m_last_event_position, m_last_event_count, m_last_event_payload);
				break;
			case TYPE_MOVE:
				m_wrapped->on_moved(m_last_event_position, m_last_event_count);
				break;
		}
		m_last_event_type = TYPE_NONE;
		m_last_event_position = -1;
		m_last_event_count = -1;
		m_last_event_payload = nullptr;
	}
};

template <typename T>
class SortedListCore {
public:
	static constexpr int INVALID_POSITION = -1;

	explicit SortedListCore(SortedListCoreCallback<T> *p_callback) :
			m_callback(p_callback) {}

	int size() const { return m_size; }

	int add(const T &p_item) {
		if (throw_if_in_mutation_operation()) {
			return INVALID_POSITION;
		}
		return add(p_item, true);
	}

	void add_all(const Vector<T> &p_items) {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		if (p_items.is_empty()) {
			return;
		}
		Vector<T> copy = p_items;
		add_all_internal(copy);
	}

	void replace_all(const Vector<T> &p_items) {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		Vector<T> copy = p_items;
		replace_all_internal(copy);
	}

	bool remove(const T &p_item) {
		if (throw_if_in_mutation_operation()) {
			return false;
		}
		return remove(p_item, true);
	}

	T remove_item_at(int p_index) {
		if (throw_if_in_mutation_operation()) {
			return T();
		}
		T item = get(p_index);
		remove_item_at_index(p_index, true);
		return item;
	}

	void update_item_at(int p_index, const T &p_item) {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		const T existing = get(p_index);
		// Assume changed if the same value is given back.
		bool contents_changed = (existing == p_item) || !m_callback->are_contents_the_same(existing, p_item);
		if (!(existing == p_item)) {
			// Different items, comparison may avoid a full lookup.
			const int cmp = m_callback->compare(existing, p_item);
			if (cmp == 0) {
				m_data.write[p_index] = p_item;
				if (contents_changed) {
					m_callback->on_changed(p_index, 1, m_callback->get_change_payload(existing, p_item));
				}
				return;
			}
		}
		if (contents_changed) {
			m_callback->on_changed(p_index, 1, m_callback->get_change_payload(existing, p_item));
		}
		remove_item_at_index(p_index, false);
		int new_index = add(p_item, false);
		if (p_index != new_index) {
			m_callback->on_moved(p_index, new_index);
		}
	}

	void recalculate_position_of_item_at(int p_index) {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		const T item = get(p_index);
		remove_item_at_index(p_index, false);
		int new_index = add(item, false);
		if (p_index != new_index) {
			m_callback->on_moved(p_index, new_index);
		}
	}

	const T &get(int p_index) const {
		// Bounds are checked by the caller-facing layers; index is assumed valid here.
		if (m_old_data.size() > 0) {
			// Call made from a callback during addAll/replaceAll execution. Data is split
			// between m_data and m_old_data.
			if (p_index >= m_new_data_start) {
				return m_old_data[p_index - m_new_data_start + m_old_data_start];
			}
		}
		return m_data[p_index];
	}

	int index_of(const T &p_item) const {
		if (m_old_data.size() > 0) {
			int index = find_index_of(p_item, m_data, 0, m_new_data_start, LOOKUP);
			if (index != INVALID_POSITION) {
				return index;
			}
			index = find_index_of(p_item, m_old_data, m_old_data_start, m_old_data_size, LOOKUP);
			if (index != INVALID_POSITION) {
				return index - m_old_data_start + m_new_data_start;
			}
			return INVALID_POSITION;
		}
		return find_index_of(p_item, m_data, 0, m_size, LOOKUP);
	}

	void clear() {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		if (m_size == 0) {
			return;
		}
		const int prev_size = m_size;
		m_size = 0;
		m_callback->on_removed(0, prev_size);
	}

	void begin_batched_updates() {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		if (m_callback_is_batched) {
			return;
		}
		m_batched_callback = SortedListCoreBatchedCallback<T>(m_callback);
		m_callback = &m_batched_callback;
		m_callback_is_batched = true;
	}

	void end_batched_updates() {
		if (throw_if_in_mutation_operation()) {
			return;
		}
		if (m_callback_is_batched) {
			m_batched_callback.dispatch_last_event();
			m_callback = m_batched_callback.get_wrapped_callback();
			m_callback_is_batched = false;
		}
	}

	// Exposed for tests: the raw backing array and capacity.
	int capacity() const { return m_data.size(); }
	const T *raw_data() const { return m_data.ptr(); }
	bool is_in_mutation() const { return m_old_data.size() > 0; }

private:
	static const int MIN_CAPACITY = 10;
	static const int CAPACITY_GROWTH = MIN_CAPACITY;
	static const int INSERTION = 1;
	static const int DELETION = 1 << 1;
	static const int LOOKUP = 1 << 2;

	SortedListCoreCallback<T> *m_callback;
	Vector<T> m_data;
	Vector<T> m_old_data;
	int m_old_data_start = 0;
	int m_old_data_size = 0;
	int m_new_data_start = 0;
	int m_size = 0;
	SortedListCoreBatchedCallback<T> m_batched_callback;
	bool m_callback_is_batched = false;

	bool throw_if_in_mutation_operation() {
		return m_old_data.size() > 0;
	}

	void add_all_internal(Vector<T> &p_new_items) {
		if (p_new_items.size() < 1) {
			return;
		}
		const int new_size = sort_and_dedup(p_new_items);
		if (m_size == 0) {
			m_data = p_new_items;
			m_size = new_size;
			m_callback->on_inserted(0, new_size);
		} else {
			merge(p_new_items, new_size);
		}
	}

	void replace_all_internal(Vector<T> &p_new_data) {
		const bool force_batched_updates = !m_callback_is_batched;
		if (force_batched_updates) {
			begin_batched_updates();
		}

		m_old_data_start = 0;
		m_old_data_size = m_size;
		m_old_data = m_data;

		m_new_data_start = 0;
		int new_size = sort_and_dedup(p_new_data);
		m_data.resize(new_size);

		while (m_new_data_start < new_size || m_old_data_start < m_old_data_size) {
			if (m_old_data_start >= m_old_data_size) {
				int insert_index = m_new_data_start;
				int item_count = new_size - m_new_data_start;
				for (int i = 0; i < item_count; i++) {
					m_data.write[insert_index + i] = p_new_data[insert_index + i];
				}
				m_new_data_start += item_count;
				m_size += item_count;
				m_callback->on_inserted(insert_index, item_count);
				break;
			}
			if (m_new_data_start >= new_size) {
				int item_count = m_old_data_size - m_old_data_start;
				m_size -= item_count;
				m_callback->on_removed(m_new_data_start, item_count);
				break;
			}

			T old_item = m_old_data[m_old_data_start];
			T new_item = p_new_data[m_new_data_start];

			int result = m_callback->compare(old_item, new_item);
			if (result < 0) {
				replace_all_remove();
			} else if (result > 0) {
				replace_all_insert(new_item);
			} else {
				if (!m_callback->are_items_the_same(old_item, new_item)) {
					// The items aren't the same even though they were supposed to occupy the same
					// place, so both notify to remove and add an item in the current location.
					replace_all_remove();
					replace_all_insert(new_item);
				} else {
					m_data.write[m_new_data_start] = new_item;
					m_old_data_start++;
					m_new_data_start++;
					if (!m_callback->are_contents_the_same(old_item, new_item)) {
						m_callback->on_changed(m_new_data_start - 1, 1, m_callback->get_change_payload(old_item, new_item));
					}
				}
			}
		}

		m_old_data.clear();
		m_old_data_start = 0;
		m_old_data_size = 0;
		m_new_data_start = 0;

		if (force_batched_updates) {
			end_batched_updates();
		}
	}

	void replace_all_insert(const T &p_new_item) {
		m_data.write[m_new_data_start] = p_new_item;
		m_new_data_start++;
		m_size++;
		m_callback->on_inserted(m_new_data_start - 1, 1);
	}

	void replace_all_remove() {
		m_size--;
		m_old_data_start++;
		m_callback->on_removed(m_new_data_start, 1);
	}

	// Stable merge sort so items with equal sort order keep their input order,
	// matching Java's TimSort-based Arrays.sort.
	void stable_sort(T *p_data, T *p_tmp, int p_count) {
		for (int width = 1; width < p_count; width *= 2) {
			for (int i = 0; i < p_count; i += width * 2) {
				int lo = i;
				int mid = (i + width) < p_count ? (i + width) : p_count;
				int hi = (i + width * 2) < p_count ? (i + width * 2) : p_count;
				int a = lo;
				int b = mid;
				int out = lo;
				while (a < mid && b < hi) {
					if (m_callback->compare(p_data[b], p_data[a]) < 0) {
						p_tmp[out++] = p_data[b++];
					} else {
						p_tmp[out++] = p_data[a++];
					}
				}
				while (a < mid) {
					p_tmp[out++] = p_data[a++];
				}
				while (b < hi) {
					p_tmp[out++] = p_data[b++];
				}
			}
			for (int k = 0; k < p_count; k++) {
				p_data[k] = p_tmp[k];
			}
		}
	}

	int sort_and_dedup(Vector<T> &p_items) {
		if (p_items.is_empty()) {
			return 0;
		}
		Vector<T> tmp;
		tmp.resize(p_items.size());
		stable_sort(p_items.ptrw(), tmp.ptrw(), p_items.size());

		// Keep track of the range of equal items at the end of the output.
		// Start with the range containing just the first item.
		int range_start = 0;
		int range_end = 1;

		for (int i = 1; i < p_items.size(); ++i) {
			T current_item = p_items[i];
			int compare = m_callback->compare(p_items[range_start], current_item);
			if (compare == 0) {
				// The range of equal items continues, update it.
				const int same_item_pos = find_same_item(current_item, p_items, range_start, range_end);
				if (same_item_pos != INVALID_POSITION) {
					// Replace the duplicate item.
					p_items.write[same_item_pos] = current_item;
				} else {
					// Expand the range.
					if (range_end != i) {
						p_items.write[range_end] = current_item;
					}
					range_end++;
				}
			} else {
				// The range has ended. Reset it to contain just the current item.
				if (range_end != i) {
					p_items.write[range_end] = current_item;
				}
				range_start = range_end++;
			}
		}
		return range_end;
	}

	int find_same_item(const T &p_item, Vector<T> &p_items, int p_from, int p_to) const {
		for (int pos = p_from; pos < p_to; pos++) {
			if (m_callback->are_items_the_same(p_items[pos], p_item)) {
				return pos;
			}
		}
		return INVALID_POSITION;
	}

	// Assumes new_items are sorted and deduplicated.
	void merge(Vector<T> &p_new_data, int p_new_data_size) {
		const bool force_batched_updates = !m_callback_is_batched;
		if (force_batched_updates) {
			begin_batched_updates();
		}

		m_old_data = m_data;
		m_old_data_start = 0;
		m_old_data_size = m_size;

		const int merged_capacity = m_size + p_new_data_size + CAPACITY_GROWTH;
		m_data.resize(merged_capacity);
		m_new_data_start = 0;

		int new_data_start = 0;
		while (m_old_data_start < m_old_data_size || new_data_start < p_new_data_size) {
			if (m_old_data_start == m_old_data_size) {
				// No more old items, copy the remaining new items.
				int item_count = p_new_data_size - new_data_start;
				for (int i = 0; i < item_count; i++) {
					m_data.write[m_new_data_start + i] = p_new_data[new_data_start + i];
				}
				m_new_data_start += item_count;
				m_size += item_count;
				m_callback->on_inserted(m_new_data_start - item_count, item_count);
				break;
			}

			if (new_data_start == p_new_data_size) {
				// No more new items, copy the remaining old items.
				int item_count = m_old_data_size - m_old_data_start;
				for (int i = 0; i < item_count; i++) {
					m_data.write[m_new_data_start + i] = m_old_data[m_old_data_start + i];
				}
				m_new_data_start += item_count;
				break;
			}

			T old_item = m_old_data[m_old_data_start];
			T new_item = p_new_data[new_data_start];
			int compare = m_callback->compare(old_item, new_item);
			if (compare > 0) {
				// New item is lower, output it.
				m_data.write[m_new_data_start++] = new_item;
				m_size++;
				new_data_start++;
				m_callback->on_inserted(m_new_data_start - 1, 1);
			} else if (compare == 0 && m_callback->are_items_the_same(old_item, new_item)) {
				// Items are the same. Output the new item, but consume both.
				m_data.write[m_new_data_start++] = new_item;
				new_data_start++;
				m_old_data_start++;
				if (!m_callback->are_contents_the_same(old_item, new_item)) {
					m_callback->on_changed(m_new_data_start - 1, 1, m_callback->get_change_payload(old_item, new_item));
				}
			} else {
				// Old item is lower than or equal to (but not the same as the new). Output it.
				// New item with the same sort order will be inserted later.
				m_data.write[m_new_data_start++] = old_item;
				m_old_data_start++;
			}
		}

		m_old_data.clear();
		m_old_data_start = 0;
		m_old_data_size = 0;
		m_new_data_start = 0;

		if (force_batched_updates) {
			end_batched_updates();
		}
	}

	int add(const T &p_item, bool p_notify) {
		int index = find_index_of(p_item, m_data, 0, m_size, INSERTION);
		if (index == INVALID_POSITION) {
			index = 0;
		} else if (index < m_size) {
			T existing = m_data[index];
			if (m_callback->are_items_the_same(existing, p_item)) {
				if (m_callback->are_contents_the_same(existing, p_item)) {
					// No change but still replace the item.
					m_data.write[index] = p_item;
					return index;
				} else {
					m_data.write[index] = p_item;
					m_callback->on_changed(index, 1, m_callback->get_change_payload(existing, p_item));
					return index;
				}
			}
		}
		add_to_data(index, p_item);
		if (p_notify) {
			m_callback->on_inserted(index, 1);
		}
		return index;
	}

	bool remove(const T &p_item, bool p_notify) {
		int index = find_index_of(p_item, m_data, 0, m_size, DELETION);
		if (index == INVALID_POSITION) {
			return false;
		}
		remove_item_at_index(index, p_notify);
		return true;
	}

	void remove_item_at_index(int p_index, bool p_notify) {
		for (int i = p_index; i < m_size - 1; i++) {
			m_data.write[i] = m_data[i + 1];
		}
		m_size--;
		if (p_notify) {
			m_callback->on_removed(p_index, 1);
		}
	}

	int find_index_of(const T &p_item, const Vector<T> &p_data, int p_left, int p_right, int p_reason) const {
		while (p_left < p_right) {
			const int middle = (p_left + p_right) / 2;
			T my_item = p_data[middle];
			const int cmp = m_callback->compare(my_item, p_item);
			if (cmp < 0) {
				p_left = middle + 1;
			} else if (cmp == 0) {
				if (m_callback->are_items_the_same(my_item, p_item)) {
					return middle;
				} else {
					int exact = linear_equality_search(p_item, p_data, middle, p_left, p_right);
					if (p_reason == INSERTION) {
						return exact == INVALID_POSITION ? middle : exact;
					} else {
						return exact;
					}
				}
			} else {
				p_right = middle;
			}
		}
		return p_reason == INSERTION ? p_left : INVALID_POSITION;
	}

	int linear_equality_search(const T &p_item, const Vector<T> &p_data, int p_middle, int p_left, int p_right) const {
		// Go left.
		for (int next = p_middle - 1; next >= p_left; next--) {
			T next_item = p_data[next];
			int cmp = m_callback->compare(next_item, p_item);
			if (cmp != 0) {
				break;
			}
			if (m_callback->are_items_the_same(next_item, p_item)) {
				return next;
			}
		}
		// Go right.
		for (int next = p_middle + 1; next < p_right; next++) {
			T next_item = p_data[next];
			int cmp = m_callback->compare(next_item, p_item);
			if (cmp != 0) {
				break;
			}
			if (m_callback->are_items_the_same(next_item, p_item)) {
				return next;
			}
		}
		return INVALID_POSITION;
	}

	void add_to_data(int p_index, const T &p_item) {
		if (m_size == m_data.size()) {
			// We are at the limit, enlarge.
			Vector<T> new_data;
			new_data.resize(m_data.size() + CAPACITY_GROWTH);
			for (int i = 0; i < p_index; i++) {
				new_data.write[i] = m_data[i];
			}
			new_data.write[p_index] = p_item;
			for (int i = p_index; i < m_size; i++) {
				new_data.write[i + 1] = m_data[i];
			}
			m_data = new_data;
		} else {
			// Just shift, we fit.
			for (int i = m_size; i > p_index; i--) {
				m_data.write[i] = m_data[i - 1];
			}
			m_data.write[p_index] = p_item;
		}
		m_size++;
	}
};

} // namespace godot
