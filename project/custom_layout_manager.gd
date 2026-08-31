class_name CustomLayoutManager
extends LayoutManager

# How to write your own layout in GDScript — this file is the tutorial.
#
# A custom LayoutManager is a script that extends LayoutManager and overrides
# the _*_ virtuals. It does NOT scroll or recycle by itself:
#   - the RecyclerView owns the scroll offset (get_scroll_offset()) and
#     re-runs _on_layout_children() after every offset change;
#   - the RecyclerView clamps the offset against _get_content_size();
#   - holders that leave the viewport must be recycled BY THE SCRIPT with
#     remove_item_view() + recycle_view() (the RecyclerView does not know
#     which positions your layout considers visible).
#
# What you work with inside _on_layout_children():
#   get_item_count()                      -> number of items (LayoutManager)
#   recycler_view.get_scroll_offset()     -> current scroll offset
#   recycler_view.get_viewport_size()     -> visible viewport size
#   recycler_view.get_item_extent(pos)    -> item size at a position
#   recycler_view.get_child_holder_count() / get_child_holder_at(i)
#                                         -> holders currently in the tree
#   recycler_view.get_view_for_position(pos)
#                                         -> obtain + bind a holder (reuses
#                                            the cache/pool when possible)
#   recycler_view.add_item_view(holder)   -> put a holder into the tree
#   recycler_view.set_item_view_position(holder, pos, size)
#                                         -> place the holder's control
#   recycler_view.remove_item_view(holder) + recycler_view.recycle_view(holder, pos)
#                                         -> take a holder out and recycle it

# Row height. A custom layout can also vary per position via
# recycler_view.get_item_extent(pos).
var extent := 40

# Wave parameters: rows drift sideways with a sine so the layout is visibly
# custom — the math is the whole point, swap it for anything you like.
var wave_amplitude := 24.0
var wave_frequency := 0.2


func _on_layout_children(recycler_view, state) -> void:
	# 1. Figure out which positions are visible at the current offset.
	var offset: int = recycler_view.get_scroll_offset()
	var viewport: Vector2 = recycler_view.get_viewport_size()
	var count: int = get_item_count()
	var first := maxi(0, offset / extent)
	var last := mini(count - 1, (offset + int(viewport.y)) / extent)

	# 2. Recycle every holder outside [first, last]. Iterate backwards because
	#    remove_item_view() shrinks the child list.
	for i in range(recycler_view.get_child_holder_count() - 1, -1, -1):
		var holder = recycler_view.get_child_holder_at(i)
		var pos: int = holder.get_position()
		if pos < first or pos > last:
			recycler_view.remove_item_view(holder)
			recycler_view.recycle_view(holder, pos)

	# 3. Fill the visible range. Holders already in the tree keep their data
	#    and only need re-positioning; missing ones are obtained (bound) and
	#    added.
	var present := {}
	for i in recycler_view.get_child_holder_count():
		var h = recycler_view.get_child_holder_at(i)
		present[h.get_position()] = h
	for pos in range(first, last + 1):
		var holder = present.get(pos)
		if holder == null:
			holder = recycler_view.get_view_for_position(pos)
			recycler_view.add_item_view(holder)
		# The wave: x drifts sideways with the position; y is the linear row.
		var wave := sin(pos * wave_frequency) * wave_amplitude
		recycler_view.set_item_view_position(holder,
				Vector2(wave, pos * extent - offset),
				Vector2(viewport.x - wave_amplitude * 2.0, extent))


func _can_scroll_vertically() -> bool:
	return true


func _get_content_size(recycler_view) -> int:
	# The scroll range: total content length. The RecyclerView clamps the
	# offset to content − viewport with this, so it must cover every row.
	return get_item_count() * extent


func _get_position_offset(position: int) -> int:
	# Where a position starts in content space. Drives scroll_to_position()
	# and the SnapHelpers. Uniform rows: position * extent.
	return position * extent


func _get_item_rect(recycler_view, position: int) -> Rect2:
	# The item's viewport-space rect (before decoration insets). Used by
	# ItemDecorations to draw dividers/spacing and by item animations.
	var wave := sin(position * wave_frequency) * wave_amplitude
	return Rect2(wave, position * extent - recycler_view.get_scroll_offset(),
			recycler_view.get_viewport_size().x, extent)


func _on_data_changed() -> void:
	# Adapter contents changed: drop any cached layout state (here none, but
	# a real custom layout would invalidate its position table).
	pass


func _collect_adjacent_prefetch_positions(dy: int) -> Array:
	# Optional: return positions just ahead of the viewport so the RecyclerView
	# pre-creates their views (scrolling there then reuses them instead of
	# building fresh ones). Without this method prefetch is simply skipped.
	if dy <= 0:
		return []
	var rv = get_recycler_view()
	var next: int = rv.get_scroll_offset() / extent + int(rv.get_viewport_size().y) / extent + 1
	return [next + 1, next + 2]
