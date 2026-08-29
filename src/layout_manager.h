#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class RecyclerView;
class State;

// Port of RecyclerView.LayoutManager. Abstract base class for the layout
// strategy that positions items inside a RecyclerView. Concrete managers
// (e.g. LinearLayoutManager) drive item obtain/recycle and positioning.
class LayoutManager : public RefCounted {
	GDCLASS(LayoutManager, RefCounted)

protected:
	static void _bind_methods();

public:
	// Called by the RecyclerView to lay out items for the current scroll offset.
	virtual void on_layout_children(RecyclerView *p_recycler_view, State *p_state) = 0;

	virtual bool can_scroll_vertically() const { return false; }
	virtual bool can_scroll_horizontally() const { return false; }

	// Attempts to scroll by the given delta; returns the consumed amount.
	virtual int scroll_vertically_by(int p_dy, RecyclerView *p_recycler_view, State *p_state) { return 0; }
	virtual int scroll_horizontally_by(int p_dx, RecyclerView *p_recycler_view, State *p_state) { return 0; }

	// Total content size along the scroll axis (drives scroll bounds).
	virtual int get_content_size(RecyclerView *p_recycler_view) const { return 0; }

	// Called when the adapter contents change, so cached layout state (e.g. the
	// cumulative height table) can be invalidated before the next layout.
	virtual void on_data_changed() {}

	// The un-inflated rect of the item at the position (before decoration insets).
	// Used by ItemDecorations to draw dividers/spacing.
	virtual Rect2 get_item_rect(RecyclerView *p_recycler_view, int p_position) const { return Rect2(); }

	// The scroll offset at which the item at the position starts along the scroll
	// axis (content space). SnapHelper settles a position by scrolling to this
	// offset (plus a centering adjustment).
	virtual int get_position_offset(int p_position) const { return 0; }

	// Collects positions adjacent to the viewport in the given scroll direction
	// (p_dy > 0 = scrolling down/right). The RecyclerView pre-creates these into
	// the recycled pool so scrolling there does not instantiate new views.
	// Mirrors LayoutManager.collectAdjacentPrefetchPositions.
	virtual void collect_adjacent_prefetch_positions(int p_dy, RecyclerView *p_recycler_view, Array &r_positions) const {}

	RecyclerView *get_recycler_view() const { return m_recycler_view; }
	void set_recycler_view(RecyclerView *p_recycler_view);

	int get_item_count() const;

private:
	RecyclerView *m_recycler_view = nullptr;
};

} // namespace godot
