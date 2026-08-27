#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/vector4.hpp>

namespace godot {

class RecyclerView;

// Port of RecyclerView.ItemDecoration. GDScript subclasses override
// _get_item_offsets to inset items (e.g. divider spacing) and _on_draw to draw
// decoration content (e.g. divider lines) beneath the item views.
class ItemDecoration : public RefCounted {
	GDCLASS(ItemDecoration, RefCounted)

protected:
	static void _bind_methods();

public:
	// Returns (left, top, right, bottom) insets for the item at the position.
	GDVIRTUAL2R(Vector4, _get_item_offsets, int, Control *)
	// Called when the RecyclerView draws, before its item views.
	GDVIRTUAL1(_on_draw, Control *)

	Vector4 get_item_offsets(int p_position, Control *p_parent);
	void on_draw(Control *p_parent);
};

} // namespace godot
