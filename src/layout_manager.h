#pragma once

#include "state.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class RecyclerView;

// Port of RecyclerView.LayoutManager. Base class for the layout strategy that
// positions items inside a RecyclerView. Concrete managers (LinearLayoutManager,
// GridLayoutManager, ...) drive item obtain/recycle and positioning.
//
// A GDScript subclass can implement its own layout by extending LayoutManager
// directly and overriding the _on_* virtuals below (script layouts do not
// scroll or recycle by themselves: the RecyclerView owns the scroll offset —
// read it with get_scroll_offset() — and re-runs _on_layout_children after
// every offset change; holders that leave the viewport must be recycled by the
// script with remove_item_view() + recycle_view()).
class LayoutManager : public RefCounted {
	GDCLASS(LayoutManager, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script overridables (Android's LayoutManager virtuals, renamed to the
	// GDScript convention):
	//
	// _on_layout_children(recycler_view, state): lay out the items visible at
	// the current scroll offset. Required for a custom layout. The script
	// positions holders with recycler_view.add_item_view() /
	// set_item_view_position(); use recycler_view.get_view_for_position() to
	// obtain (and bind) a holder, recycler_view.remove_item_view() +
	// recycler_view.recycle_view() to recycle one that left the viewport, and
	// get_item_count() / get_scroll_offset() / get_viewport_size() /
	// get_item_extent() to read the inputs.
	// Mirrors LayoutManager.onLayoutChildren.
	GDVIRTUAL2_REQUIRED(_on_layout_children, Object *, Ref<State>)
	// Mirrors LayoutManager.canScrollVertically / canScrollHorizontally
	// (default false).
	GDVIRTUAL0RC(bool, _can_scroll_vertically)
	GDVIRTUAL0RC(bool, _can_scroll_horizontally)
	// Mirrors LayoutManager.computeVerticalScrollRange: total content size
	// along the scroll axis (default 0, which clamps the list to one viewport).
	GDVIRTUAL1RC(int, _get_content_size, Object *)
	// Mirrors LayoutManager.getDecoratedBoundsWithMargins: the un-inset rect of
	// the item at the position, used by ItemDecorations and item animations.
	GDVIRTUAL2RC(Rect2, _get_item_rect, Object *, int)
	// Mirrors LayoutManager.getPositionOffset: the content offset at which the
	// item starts. Drives scroll_to_position() and the SnapHelpers.
	GDVIRTUAL1RC(int, _get_position_offset, int)
	// Mirrors LayoutManager.onItemsChanged: called when the adapter contents
	// change, so cached layout state can be invalidated.
	GDVIRTUAL0(_on_data_changed)
	// Mirrors LayoutManager.collectAdjacentPrefetchPositions: returns positions
	// ahead of the viewport (p_dy > 0 = scrolling down/right) so the
	// RecyclerView can pre-create their views into the recycled pool. Use
	// get_recycler_view() to inspect the list. (Returned as a value because
	// out-parameter arrays do not cross the script boundary.)
	GDVIRTUAL1RC(Array, _collect_adjacent_prefetch_positions, int)
	// Called by the RecyclerView to lay out items for the current scroll offset.
	// Base implementation dispatches to the script's _on_layout_children (and
	// prints a warning when a script subclass does not override it).
	virtual void on_layout_children(RecyclerView *p_recycler_view, State *p_state);

	virtual bool can_scroll_vertically() const;
	virtual bool can_scroll_horizontally() const;

	// Attempts to scroll by the given delta; returns the consumed amount.
	// NOTE: this port drives scrolling through a single shared offset space on
	// the RecyclerView (get_scroll_offset / set_scroll_offset), so these
	// Android-style hooks are never called here; a script layout only needs to
	// react to offset changes in _on_layout_children.
	virtual int scroll_vertically_by(int p_dy, RecyclerView *p_recycler_view, State *p_state) { return 0; }
	virtual int scroll_horizontally_by(int p_dx, RecyclerView *p_recycler_view, State *p_state) { return 0; }

	// Total content size along the scroll axis (drives scroll bounds).
	virtual int get_content_size(RecyclerView *p_recycler_view) const;

	// Called when the adapter contents change, so cached layout state (e.g. the
	// cumulative height table) can be invalidated before the next layout.
	virtual void on_data_changed();

	// The un-inflated rect of the item at the position (before decoration insets).
	// Used by ItemDecorations to draw dividers/spacing.
	virtual Rect2 get_item_rect(RecyclerView *p_recycler_view, int p_position) const;

	// The scroll offset at which the item at the position starts along the scroll
	// axis (content space). SnapHelper settles a position by scrolling to this
	// offset (plus a centering adjustment); scroll_to_position scrolls to it so
	// the item's leading edge aligns the viewport start (or its trailing edge the
	// viewport end under reverse_layout).
	virtual int get_position_offset(int p_position) const;

	// Reverse layout: items are laid out from the trailing edge (bottom for a
	// vertical list) instead of the leading edge, and scrolling toward the scroll
	// offset max shows the last item. Mirrors LinearLayoutManager.reverseLayout.
	// The scroll offset space itself is unchanged (0 = content start), only the
	// content->screen mapping flips.
	void set_reverse_layout(bool p_reverse);
	bool get_reverse_layout() const { return m_reverse_layout; }
	bool is_reverse_layout() const { return m_reverse_layout; }

	// Collects positions adjacent to the viewport in the given scroll direction
	// (p_dy > 0 = scrolling down/right). The RecyclerView pre-creates these into
	// the recycled pool so scrolling there does not instantiate new views.
	// Mirrors LayoutManager.collectAdjacentPrefetchPositions.
	virtual void collect_adjacent_prefetch_positions(int p_dy, RecyclerView *p_recycler_view, Array &r_positions) const;

	RecyclerView *get_recycler_view() const { return m_recycler_view; }
	void set_recycler_view(RecyclerView *p_recycler_view);

	int get_item_count() const;

private:
	RecyclerView *m_recycler_view = nullptr;
	bool m_reverse_layout = false;
};

} // namespace godot
