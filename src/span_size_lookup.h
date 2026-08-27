#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

// Port of GridLayoutManager.SpanSizeLookup. GDScript subclasses override
// _get_span_size to report how many grid columns an item occupies; the default
// is one span per item.
class SpanSizeLookup : public RefCounted {
	GDCLASS(SpanSizeLookup, RefCounted)

protected:
	static void _bind_methods();

public:
	GDVIRTUAL1R(int, _get_span_size, int)

	// Returns the number of spans the item at the position occupies (>= 1).
	int get_span_size(int p_position);
};

} // namespace godot
