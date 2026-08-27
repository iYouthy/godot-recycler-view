#include "scroll_listener.h"

namespace godot {

void ScrollListener::_bind_methods() {
	GDVIRTUAL_BIND(_on_scroll_state_changed, "state");
	GDVIRTUAL_BIND(_on_scrolled, "dx", "dy");
	ClassDB::bind_method(D_METHOD("on_scroll_state_changed", "state"), &ScrollListener::on_scroll_state_changed);
	ClassDB::bind_method(D_METHOD("on_scrolled", "dx", "dy"), &ScrollListener::on_scrolled);
}

void ScrollListener::on_scroll_state_changed(int p_state) {
	GDVIRTUAL_CALL(_on_scroll_state_changed, p_state);
}

void ScrollListener::on_scrolled(int p_dx, int p_dy) {
	GDVIRTUAL_CALL(_on_scrolled, p_dx, p_dy);
}

} // namespace godot
