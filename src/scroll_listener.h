#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

// Android RecyclerView.OnScrollListener: receives scroll position deltas on
// every scroll step and state transitions between IDLE / DRAGGING / SETTLING.
class ScrollListener : public RefCounted {
	GDCLASS(ScrollListener, RefCounted)

public:
	virtual void on_scroll_state_changed(int p_state);
	virtual void on_scrolled(int p_dx, int p_dy);

protected:
	static void _bind_methods();

	GDVIRTUAL1(_on_scroll_state_changed, int)
	GDVIRTUAL2(_on_scrolled, int, int)
};

} // namespace godot
