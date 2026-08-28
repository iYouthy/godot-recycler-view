#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of DiffUtil.ItemCallback. Compares individual list items (the item
// objects themselves), unlike DiffUtilCallback which compares by position.
// ListAdapter uses this to diff its internally-held list on submit_list().
class DiffUtilItemCallback : public RefCounted {
	GDCLASS(DiffUtilItemCallback, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script-overridable virtuals, mirroring DiffUtil.ItemCallback.
	GDVIRTUAL2R(bool, _are_items_the_same, Variant, Variant)
	GDVIRTUAL2R(bool, _are_contents_the_same, Variant, Variant)
	GDVIRTUAL2R(Variant, _get_change_payload, Variant, Variant)

	// C++ wrappers called by ListAdapter's internal diff bridge.
	bool are_items_the_same(const Variant &p_old_item, const Variant &p_new_item);
	bool are_contents_the_same(const Variant &p_old_item, const Variant &p_new_item);
	Variant get_change_payload(const Variant &p_old_item, const Variant &p_new_item);
};

} // namespace godot
