#pragma once

#include "sorted_list.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of SortedList.Callback. Abstract base class; GDScript subclasses override
// the _compare / _are_* / _on_* methods to describe their items.
class SortedListCallback : public RefCounted, public SortedListCoreCallback<Variant> {
	GDCLASS(SortedListCallback, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script-overridable virtual methods.
	GDVIRTUAL2R(int, _compare, Variant, Variant)
	GDVIRTUAL2R(bool, _are_items_the_same, Variant, Variant)
	GDVIRTUAL2R(bool, _are_contents_the_same, Variant, Variant)
	GDVIRTUAL2R(Variant, _get_change_payload, Variant, Variant)
	GDVIRTUAL2(_on_inserted, int, int)
	GDVIRTUAL2(_on_removed, int, int)
	GDVIRTUAL2(_on_moved, int, int)
	GDVIRTUAL2(_on_changed, int, int)
	GDVIRTUAL3(_on_changed_with_payload, int, int, Variant)

	// SortedListCoreCallback<Variant> implementation (drives the pure algorithm).
	int compare(const Variant &p_o1, const Variant &p_o2) override;
	bool are_items_the_same(const Variant &p_item1, const Variant &p_item2) override;
	bool are_contents_the_same(const Variant &p_old_item, const Variant &p_new_item) override;
	const void *get_change_payload(const Variant &p_item1, const Variant &p_item2) override;
	void on_inserted(int p_position, int p_count) override;
	void on_removed(int p_position, int p_count) override;
	void on_moved(int p_from_position, int p_to_position) override;
	void on_changed(int p_position, int p_count, const void *p_payload) override;

private:
	Variant m_payload_buffer;
};

// Port of SortedList. Holds items in sorted order, dispatching change
// notifications to the callback.
class SortedList : public RefCounted {
	GDCLASS(SortedList, RefCounted)

protected:
	static void _bind_methods();

public:
	enum {
		INVALID_POSITION = -1,
	};

	SortedList();
	~SortedList() override;

	void set_callback(const Ref<SortedListCallback> &p_callback);
	Ref<SortedListCallback> get_callback() const;

	int size() const;
	int add(const Variant &p_item);
	void add_all(const Array &p_items);
	void replace_all(const Array &p_items);
	bool remove(const Variant &p_item);
	Variant remove_item_at(int p_index);
	void update_item_at(int p_index, const Variant &p_item);
	void recalculate_position_of_item_at(int p_index);
	Variant get(int p_index) const;
	int index_of(const Variant &p_item) const;
	void clear();
	void begin_batched_updates();
	void end_batched_updates();

private:
	Ref<SortedListCallback> m_callback;
	SortedListCore<Variant> *m_core = nullptr;
};

} // namespace godot
