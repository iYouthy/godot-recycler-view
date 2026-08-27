#include "item_decoration.h"

namespace godot {

void ItemDecoration::_bind_methods() {
	GDVIRTUAL_BIND(_get_item_offsets, "position", "parent");
	GDVIRTUAL_BIND(_on_draw, "parent");
	ClassDB::bind_method(D_METHOD("get_item_offsets", "position", "parent"), &ItemDecoration::get_item_offsets);
	ClassDB::bind_method(D_METHOD("on_draw", "parent"), &ItemDecoration::on_draw);
}

Vector4 ItemDecoration::get_item_offsets(int p_position, Control *p_parent) {
	Vector4 result;
	if (!GDVIRTUAL_CALL(_get_item_offsets, p_position, p_parent, result)) {
		return Vector4();
	}
	return result;
}

void ItemDecoration::on_draw(Control *p_parent) {
	GDVIRTUAL_CALL(_on_draw, p_parent);
}

} // namespace godot
