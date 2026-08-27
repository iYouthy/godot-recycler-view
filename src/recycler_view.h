#pragma once

#include "adapter.h"
#include "adapter_helper.h"
#include "item_decoration.h"
#include "layout_manager.h"
#include "recycler.h"
#include "state.h"
#include "view_holder.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector4.hpp>

namespace godot {

// Port of RecyclerView (minimal hub). A Control that lays out items provided by
// an Adapter via a LayoutManager, recycling ViewHolders that scroll out of the
// visible area. Item controls are positioned absolutely and clipped.
class RecyclerView : public Control {
	GDCLASS(RecyclerView, Control)

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	RecyclerView();
	~RecyclerView() override;

	void _gui_input(const Ref<InputEvent> &p_event) override;
	void _draw();

	void set_adapter(const Ref<Adapter> &p_adapter);
	Ref<Adapter> get_adapter() const;

	void set_layout(const Ref<LayoutManager> &p_layout);
	Ref<LayoutManager> get_layout() const;

	Ref<State> get_state() const { return m_state; }
	Ref<Recycler> get_recycler() const { return m_recycler; }
	Ref<AdapterHelper> get_adapter_helper() const { return m_adapter_helper; }

	// Adapter-driven incremental updates (queue an op then re-layout).
	void notify_item_range_changed(int p_position, int p_count, const Variant &p_payload);
	void notify_item_range_inserted(int p_position, int p_count);
	void notify_item_range_removed(int p_position, int p_count);
	void notify_item_moved(int p_from_position, int p_to_position);
	// Full data-set change (notify_data_set_changed): clears queued updates and
	// re-lays out everything.
	void notify_data_changed();

	// Item view plumbing used by LayoutManagers.
	Ref<ViewHolder> get_view_for_position(int p_position);
	void recycle_view(const Ref<ViewHolder> &p_holder, int p_position);
	void add_item_view(const Ref<ViewHolder> &p_holder);
	void remove_item_view(const Ref<ViewHolder> &p_holder);
	void set_item_view_position(const Ref<ViewHolder> &p_holder, const Vector2 &p_pos, const Vector2 &p_size);
	int get_child_holder_count() const { return m_children.size(); }
	Ref<ViewHolder> get_child_holder_at(int p_index) const;

	int get_scroll_offset() const { return m_scroll_offset; }
	int get_scroll_offset_horizontal() const { return m_scroll_offset_h; }
	void set_scroll_offset(int p_offset);
	void set_scroll_offset_horizontal(int p_offset);
	void scroll_vertically(int p_delta);
	void scroll_horizontally(int p_delta);
	void scroll_along_axis(int p_delta);
	Vector2 get_viewport_size() const;

	// In a horizontal layout, whether the vertical mouse wheel also drives
	// horizontal scrolling (default true). When false, only WHEEL_LEFT/RIGHT
	// (touchpad swipe, Shift+wheel) scroll horizontally.
	void set_vertical_wheel_scrolls_horizontal(bool p_enabled);
	bool get_vertical_wheel_scrolls_horizontal() const { return m_vertical_wheel_scrolls_horizontal; }

	void set_item_size(int p_size);
	int get_item_size() const { return m_item_size; }
	// Height of the item at the given position along the scroll axis: the
	// adapter's variable height, or the default item size when not provided.
	int get_item_height(int p_position) const;

	// Item decorations (dividers, spacing).
	void add_item_decoration(const Ref<ItemDecoration> &p_decor);
	void remove_item_decoration(const Ref<ItemDecoration> &p_decor);
	int get_item_decoration_count() const { return m_decorations.size(); }
	// Accumulated (left, top, right, bottom) insets for the position.
	Vector4 get_item_insets(int p_position) const;
	// The position's actual drawn rect after insets are applied.
	Rect2 get_decorated_item_rect(int p_position) const;

	void layout_children();
	void request_layout();
	// Frees every item Control (visible and recycled). Teardown helper.
	void free_items();

private:
	void detach_from_adapter();
	void attach_to_adapter();
	void process_pending_updates();
	void mark_data_changed();

	Ref<Adapter> m_adapter;
	Ref<LayoutManager> m_layout;
	Ref<Recycler> m_recycler;
	Ref<State> m_state;
	Ref<AdapterDataObserver> m_data_observer;
	Ref<AdapterHelper> m_adapter_helper;
	Vector<Ref<ItemDecoration>> m_decorations;

	// Tracked child ViewHolders (in tree order), for recycling on scroll.
	Vector<Ref<ViewHolder>> m_children;

	int m_scroll_offset = 0;
	int m_scroll_offset_h = 0;
	// Item size along the scroll axis, used by the LayoutManager. In a full port
	// this is derived from the adapter/measurement; fixed for the first slice.
	int m_item_size = 64;
	bool m_layout_in_progress = false;

	bool m_dragging = false;
	int m_drag_start_mouse = 0;
	int m_drag_start_mouse_x = 0;
	int m_drag_start_scroll = 0;
	int m_drag_start_scroll_h = 0;
	bool m_drag_scrolled = false;
	bool m_vertical_wheel_scrolls_horizontal = true;
};

} // namespace godot
