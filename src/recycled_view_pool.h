#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of RecyclerView.RecycledViewPool. Holds recycled item handles per view
// type, bounded by a per-type capacity. Items are opaque handles (in Godot, the
// wrapped Control of a ViewHolder). put_recycled_view takes the view type
// explicitly because the handle itself is opaque.

class RecycledViewPool {
public:
	struct ScrapData {
		Vector<void *> scrap_heap;
		int max_scrap = DEFAULT_MAX_SCRAP;
		int64_t create_running_average_ns = 0;
		int64_t bind_running_average_ns = 0;
	};

	// Discards all pooled items, preserving per-type capacity settings.
	void clear() {
		for (int i = 0; i < m_scrap_data.size(); i++) {
			m_scrap_data.write[i].scrap_heap.clear();
		}
	}

	// Sets the maximum number of items to hold for the type, trimming excess.
	void set_max_recycled_views(int p_view_type, int p_max) {
		ScrapData &scrap_data = get_scrap_data_for_type(p_view_type);
		scrap_data.max_scrap = p_max;
		while (scrap_data.scrap_heap.size() > p_max) {
			scrap_data.scrap_heap.remove_at(scrap_data.scrap_heap.size() - 1);
		}
	}

	int get_recycled_view_count(int p_view_type) const {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				return m_scrap_data[i].scrap_heap.size();
			}
		}
		return 0;
	}

	// Per-type capacity (DEFAULT_MAX_SCRAP if the type has no scrap data yet).
	int get_max_recycled_views(int p_view_type) const {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				return m_scrap_data[i].max_scrap;
			}
		}
		return DEFAULT_MAX_SCRAP;
	}

	// Pops the most recently pooled item of the type, or nullptr if none.
	void *get_recycled_view(int p_view_type) {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				Vector<void *> &heap = m_scrap_data.write[i].scrap_heap;
				if (heap.is_empty()) {
					return nullptr;
				}
				void *view = heap[heap.size() - 1];
				heap.remove_at(heap.size() - 1);
				return view;
			}
		}
		return nullptr;
	}

	// Total number of pooled items across all types.
	int size() const {
		int count = 0;
		for (int i = 0; i < m_scrap_data.size(); i++) {
			count += m_scrap_data[i].scrap_heap.size();
		}
		return count;
	}

	// Adds a scrap item to the pool. If the pool is already full for that type,
	// the item is discarded immediately.
	void put_recycled_view(void *p_view, int p_view_type) {
		ScrapData &scrap_data = get_scrap_data_for_type(p_view_type);
		if (scrap_data.max_scrap <= scrap_data.scrap_heap.size()) {
			return;
		}
		scrap_data.scrap_heap.push_back(p_view);
	}

	static int64_t running_average(int64_t p_old_average, int64_t p_new_value) {
		if (p_old_average == 0) {
			return p_new_value;
		}
		return (p_old_average / 4 * 3) + (p_new_value / 4);
	}

	void factor_in_create_time(int p_view_type, int64_t p_create_time_ns) {
		ScrapData &scrap_data = get_scrap_data_for_type(p_view_type);
		scrap_data.create_running_average_ns = running_average(scrap_data.create_running_average_ns, p_create_time_ns);
	}

	void factor_in_bind_time(int p_view_type, int64_t p_bind_time_ns) {
		ScrapData &scrap_data = get_scrap_data_for_type(p_view_type);
		scrap_data.bind_running_average_ns = running_average(scrap_data.bind_running_average_ns, p_bind_time_ns);
	}

	bool will_create_in_time(int p_view_type, int64_t p_approx_current_ns, int64_t p_deadline_ns) const {
		int64_t expected = get_create_running_average(p_view_type);
		return expected == 0 || (p_approx_current_ns + expected < p_deadline_ns);
	}

	bool will_bind_in_time(int p_view_type, int64_t p_approx_current_ns, int64_t p_deadline_ns) const {
		int64_t expected = get_bind_running_average(p_view_type);
		return expected == 0 || (p_approx_current_ns + expected < p_deadline_ns);
	}

	int attach_count() const { return m_attach_count_for_clearing; }
	void attach() { m_attach_count_for_clearing++; }
	void detach() { m_attach_count_for_clearing--; }

	int64_t get_create_running_average_ns(int p_view_type) const {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				return m_scrap_data[i].create_running_average_ns;
			}
		}
		return 0;
	}

	int64_t get_bind_running_average_ns(int p_view_type) const {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				return m_scrap_data[i].bind_running_average_ns;
			}
		}
		return 0;
	}

private:
	static const int DEFAULT_MAX_SCRAP = 5;

	int64_t get_create_running_average(int p_view_type) const {
		return get_create_running_average_ns(p_view_type);
	}

	int64_t get_bind_running_average(int p_view_type) const {
		return get_bind_running_average_ns(p_view_type);
	}

	ScrapData &get_scrap_data_for_type(int p_view_type) {
		for (int i = 0; i < m_view_types.size(); i++) {
			if (m_view_types[i] == p_view_type) {
				return m_scrap_data.write[i];
			}
		}
		m_view_types.push_back(p_view_type);
		m_scrap_data.push_back(ScrapData());
		return m_scrap_data.write[m_scrap_data.size() - 1];
	}

	Vector<int> m_view_types;
	Vector<ScrapData> m_scrap_data;
	int m_attach_count_for_clearing = 0;
};

} // namespace godot
