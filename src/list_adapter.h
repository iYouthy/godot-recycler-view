#pragma once

#include "adapter.h"
#include "adapter_list_update_callback.h"
#include "diff_util.h"
#include "diff_util_item_callback.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.ListAdapter (synchronous subset). Holds
// a list plus a DiffUtilItemCallback; submit_list() diffs the previous list
// against the new one and dispatches incremental updates to the attached
// RecyclerView automatically. Extend in GDScript to provide _create_item /
// _bind_item (and optionally _get_item_view_type / _get_item_extent /
// _get_item_id, reading data via get_item()). _get_item_count is provided:
// it returns the current list size.
class ListAdapter : public Adapter {
	GDCLASS(ListAdapter, Adapter)

protected:
	static void _bind_methods();

public:
	ListAdapter();

	// The item comparator used by submit_list() (DiffUtil.ItemCallback port).
	void set_diff_callback(const Ref<DiffUtilItemCallback> &p_callback);
	Ref<DiffUtilItemCallback> get_diff_callback() const { return m_diff_callback; }

	// Diffs the current list against p_list and dispatches incremental updates.
	// A no-op when p_list is the same Array instance as the current list.
	void submit_list(const Array &p_list);
	Array get_current_list() const { return m_list; }
	Variant get_item(int p_index) const;

	// Adapter: item count is the current list size.
	int get_item_count() override;

	// Optional hook, fired after submit_list() commits a new list.
	GDVIRTUAL2(_on_current_list_changed, Array, Array)

private:
	// Bridges the position-based DiffUtilCallback consumed by the diff algorithm
	// to the item-based DiffUtilItemCallback the user provides.
	class ItemDiffCallback : public DiffUtilCallback {
	public:
		Ref<DiffUtilItemCallback> item_callback;
		Array old_items;
		Array new_items;

		int get_old_list_size() override { return old_items.size(); }
		int get_new_list_size() override { return new_items.size(); }
		bool are_items_the_same(int p_old_item_position, int p_new_item_position) override {
			return item_callback->are_items_the_same(old_items[p_old_item_position], new_items[p_new_item_position]);
		}
		bool are_contents_the_same(int p_old_item_position, int p_new_item_position) override {
			return item_callback->are_contents_the_same(old_items[p_old_item_position], new_items[p_new_item_position]);
		}
		const void *get_change_payload(int p_old_item_position, int p_new_item_position) override;
		Variant m_payload_buffer;
	};

	Array m_list;
	Ref<DiffUtilItemCallback> m_diff_callback;
	Ref<AdapterListUpdateCallback> m_update_callback;
};

} // namespace godot
