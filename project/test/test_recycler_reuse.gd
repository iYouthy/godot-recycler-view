# Regression: incremental head-inserts must not fabricate a fresh holder per
# pass. The pushed-out bottom holder is recycled and the new head item reuses
# it, so the created counter stays bounded. Covers both the no-animator path
# (immediate recycle) and the animator path (out-of-view holders are skipped in
# the animation dispatch so their move never re-triggers forever, and they are
# recycled once their animation finishes).

extends GdUnitTestSuite


class InsAdapter extends Adapter:
	var items: Array = []
	var created := 0

	func _get_item_count() -> int:
		return items.size()

	func _get_item_extent(_p: int) -> int:
		return 40

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(position)


func _make_rv(with_animator: bool) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 200))  # 5 visible items at 40px
	var adapter := InsAdapter.new()
	for i in 5:
		adapter.items.append("i%d" % i)
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	if with_animator:
		rv.set_item_animator(DefaultItemAnimator.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_head_insert_without_animator_created_bounded() -> void:
	var s := await _make_rv(false)
	var rv: RecyclerView = s.rv
	var adapter: InsAdapter = s.adapter
	for i in 30:
		adapter.items.insert(0, "new%d" % i)
		rv.notify_item_range_inserted(0, 1)
		await get_tree().process_frame
	# 5 visible + a couple cached/pooled; far below the 30 inserts.
	assert_that(adapter.created).is_less(15)
	rv.free_items()
	rv.free()


func test_head_insert_with_animator_created_bounded() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var adapter: InsAdapter = s.adapter
	for i in 10:
		adapter.items.insert(0, "new%d" % i)
		rv.notify_item_range_inserted(0, 1)
		# Let the move animation (300ms) finish so the out-of-view holder that
		# slid past the bottom edge is recycled instead of re-animating forever.
		await get_tree().create_timer(0.4).timeout
	# Bounded by the viewport plus the animating margin, never linear in the
	# number of inserts.
	assert_that(adapter.created).is_less(15)
	assert_that(rv.get_child_holder_count()).is_less(12)
	rv.free_items()
	rv.free()


# A grid with mixed view types and uneven row heights scrolls in/out unequal
# counts per pass (a header row scrolls out while item cells scroll in). The
# recycled pool must be large enough to smooth that over, or every mismatch
# fabricates a fresh holder and the created counter grows without bound.
class LocalGridAdapter extends Adapter:
	var count: int = 0
	var created := 0

	class Lookup extends SpanSizeLookup:
		func _get_span_size(position: int) -> int:
			return 3 if position % 10 == 0 else 1

	func _get_item_count() -> int:
		return count

	func _get_item_view_type(position: int) -> int:
		return 0 if position % 10 == 0 else 1

	func _get_item_extent(position: int) -> int:
		return 80 if position % 10 == 0 else 50 + (position % 3) * 10

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(120, 60))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(position)


func test_grid_mixed_view_types_scroll_created_bounded() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 200))
	var adapter := LocalGridAdapter.new()
	adapter.count = 10000
	var layout := GridLayoutManager.new()
	layout.set_span_count(3)
	layout.set_span_size_lookup(LocalGridAdapter.Lookup.new())
	rv.set_item_extent(60)
	rv.set_adapter(adapter)
	rv.set_layout(layout)
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	for i in 60:
		rv.scroll_vertically(40)
	# Far below the 60 scrolled passes; the pool smooths the header/item mismatch.
	assert_that(adapter.created).is_less(25)
	rv.free_items()
	rv.free()


func test_scroll_and_insert_mid_animation_no_crash_created_bounded() -> void:
	# Scroll and insert without letting the move animations finish. A freshly
	# added holder fades in while a move slides it (two animations at once);
	# finishing the fade must not mark it recyclable while the move still touches
	# its control (that used to recycle it mid-animation and crash on a freed
	# control).
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(360, 600))
	var adapter := InsAdapter.new()
	for i in 10000:
		adapter.items.append(i)
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.set_item_animator(DefaultItemAnimator.new())
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	for i in 25:
		rv.scroll_vertically(200)  # 5 items per jump, animations always in flight
		adapter.items.insert(0, -i)
		rv.notify_item_range_inserted(0, 1)
		await get_tree().create_timer(0.05).timeout
	# 15 visible + a few animating; far below the 25 inserts.
	assert_that(adapter.created).is_less(45)
	rv.free_items()
	rv.free()


# Dragging the scroll bar is incremental (Android's handleScrollBarDragging):
# each motion advances the thumb by the mouse delta, so the viewport shifts only
# a fraction of its size per frame. The cached holders' positions overlap frame
# to frame, so the position-bound cache + pool absorb the recycling and no fresh
# holder is fabricated while dragging.
func test_scroll_bar_drag_created_bounded() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(600, 360))  # 15 visible items at 40px
	var adapter := InsAdapter.new()
	for i in 10000:
		adapter.items.append(i)
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.set_scroll_bar(DefaultScrollBar.new())
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	# A full drag scrolls a large range incrementally: ~4 items per frame (fast
	# drag, but always well under the 15-item viewport) for 60 frames.
	for i in 60:
		rv.set_scroll_offset_horizontal(rv.get_scroll_offset_horizontal() + 160)
		await get_tree().process_frame
	# 15 visible + cache + pool slack; far below the ~240 items scrolled over.
	assert_that(adapter.created).is_less(40)
	rv.free_items()
	rv.free()
