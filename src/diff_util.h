#pragma once

#include "diff_algo.h"
#include "list_update_callback.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.DiffUtil. Entry point for computing the
// difference between two lists.
class DiffUtil : public RefCounted {
	GDCLASS(DiffUtil, RefCounted)

protected:
	static void _bind_methods();

public:
	static Ref<class DiffResult> calculate_diff(const Ref<class DiffUtilCallback> &p_callback, bool p_detect_moves = true);
};

// Port of DiffUtil.Callback. Abstract base class; GDScript subclasses override
// the _get_* / _are_* methods to describe their lists.
class DiffUtilCallback : public RefCounted, public DiffCallback {
	GDCLASS(DiffUtilCallback, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script-overridable virtual methods.
	GDVIRTUAL0R(int, _get_old_list_size)
	GDVIRTUAL0R(int, _get_new_list_size)
	GDVIRTUAL2R(bool, _are_items_the_same, int, int)
	GDVIRTUAL2R(bool, _are_contents_the_same, int, int)
	GDVIRTUAL2R(Variant, _get_change_payload, int, int)

	// DiffCallback implementation (drives the pure algorithm).
	int get_old_list_size() override;
	int get_new_list_size() override;
	bool are_items_the_same(int p_old_item_position, int p_new_item_position) override;
	bool are_contents_the_same(int p_old_item_position, int p_new_item_position) override;
	const void *get_change_payload(int p_old_item_position, int p_new_item_position) override;

private:
	// Transient storage handed to the algorithm as an opaque payload pointer;
	// keeps the GDScript payload Variant alive during dispatch.
	Variant m_payload_buffer;
};

// Port of DiffUtil.DiffResult. Holds the computed diff and applies it to a
// ListUpdateCallback.
class DiffResult : public RefCounted {
	GDCLASS(DiffResult, RefCounted)

protected:
	static void _bind_methods();

public:
	DiffResult();
	~DiffResult() override;

	int convert_old_position_to_new(int p_old_list_position) const;
	int convert_new_position_to_old(int p_new_list_position) const;
	void dispatch_updates_to(const Ref<ListUpdateCallback> &p_update_callback) const;

private:
	friend class DiffUtil;
	Ref<DiffUtilCallback> m_callback;
	DiffResultData *m_result_data = nullptr;
};

} // namespace godot
