#include "diff_util_item_callback.h"

namespace godot {

void DiffUtilItemCallback::_bind_methods() {
	GDVIRTUAL_BIND(_are_items_the_same, "old_item", "new_item");
	GDVIRTUAL_BIND(_are_contents_the_same, "old_item", "new_item");
	GDVIRTUAL_BIND(_get_change_payload, "old_item", "new_item");
}

bool DiffUtilItemCallback::are_items_the_same(const Variant &p_old_item, const Variant &p_new_item) {
	bool result = false;
	GDVIRTUAL_CALL(_are_items_the_same, p_old_item, p_new_item, result);
	return result;
}

bool DiffUtilItemCallback::are_contents_the_same(const Variant &p_old_item, const Variant &p_new_item) {
	bool result = false;
	GDVIRTUAL_CALL(_are_contents_the_same, p_old_item, p_new_item, result);
	return result;
}

Variant DiffUtilItemCallback::get_change_payload(const Variant &p_old_item, const Variant &p_new_item) {
	Variant result;
	GDVIRTUAL_CALL(_get_change_payload, p_old_item, p_new_item, result);
	return result;
}

} // namespace godot
