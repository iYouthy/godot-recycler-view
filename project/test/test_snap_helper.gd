# Tests for the SnapHelper: LinearSnapHelper center-snap and PagerSnapHelper
# page-snap after a fling or a scroll that settled.

extends GdUnitTestSuite


class CardAdapter extends Adapter:
	var count: int = 0

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 600))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(position)


func _press(rv: RecyclerView, pos: Vector2) -> void:
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = pos
	get_tree().root.push_input(press)


func _release(rv: RecyclerView, pos: Vector2) -> void:
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = pos
	get_tree().root.push_input(rel)


func _motion(rv: RecyclerView, pos: Vector2, left_down: bool) -> void:
	var mm := InputEventMouseMotion.new()
	mm.button_mask = MOUSE_BUTTON_MASK_LEFT if left_down else 0
	mm.position = pos
	get_tree().root.push_input(mm)


func _await_idle(rv: RecyclerView, max_frames: int = 400) -> void:
	for i in max_frames:
		if rv.get_scroll_state() == RecyclerView.SCROLL_STATE_IDLE:
			return
		await get_tree().process_frame


func _make_rv(width: int, item_extent: int, count: int, helper: SnapHelper) -> RecyclerView:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(width, 600))
	var adapter := CardAdapter.new()
	adapter.count = count
	rv.set_item_extent(item_extent)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	helper.attach_to_recycler_view(rv)
	await get_tree().process_frame
	return rv


func test_pager_forward_fling_snaps_next_page() -> void:
	# One page == one card == viewport width (200). A forward fling (drag left)
	# must settle so the NEXT page's start aligns to the viewport start.
	var helper := PagerSnapHelper.new()
	var rv := await _make_rv(200, 200, 10, helper)
	rv.scroll_horizontally(50)  # start partway off-page so the fling can move

	_press(rv, Vector2(150, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(120, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(90, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(60, 100), true)
	await get_tree().process_frame
	_release(rv, Vector2(60, 100))

	await _await_idle(rv)
	var offset := rv.get_scroll_offset_horizontal()
	assert_that(offset).is_equal(200)  # page 1 start aligned, not a partial page
	rv.free_items()
	rv.free()


func test_pager_strong_fling_only_one_page() -> void:
	# The velocity magnitude must not let a fling skip pages: a strong forward
	# fling from a partly-scrolled offset still snaps one page forward.
	var helper := PagerSnapHelper.new()
	var rv := await _make_rv(200, 200, 10, helper)
	rv.scroll_horizontally(50)

	_press(rv, Vector2(150, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(60, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(30, 100), true)
	await get_tree().process_frame
	_release(rv, Vector2(30, 100))

	await _await_idle(rv)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(200)  # not 400+
	rv.free_items()
	rv.free()


func test_pager_snaps_on_idle_after_small_drag() -> void:
	# A drag below the fling threshold still snaps when it settles: the nearest
	# page center is restored.
	var helper := PagerSnapHelper.new()
	var rv := await _make_rv(200, 200, 10, helper)
	rv.scroll_horizontally(50)

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_release(rv, Vector2(100, 100))

	await _await_idle(rv)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(0)  # back to page 0
	rv.free_items()
	rv.free()


func test_linear_snaps_closest_to_center() -> void:
	# Chips 60px wide in a 200px viewport. After scrolling partway and settling,
	# the chip closest to the center is aligned with it.
	var helper := LinearSnapHelper.new()
	var rv := await _make_rv(200, 60, 20, helper)
	rv.scroll_horizontally(30)

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_release(rv, Vector2(100, 100))

	await _await_idle(rv)
	# At offset 30 the chip nearest the center is #2 (content 120); centering it
	# puts the offset at 120 + 30 - 100 = 50.
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(50)
	rv.free_items()
	rv.free()


func test_linear_strong_fling_settles_quickly() -> void:
	# A fast fling across many large items must park on the estimated page within
	# ~a second, not drag on for the full linear distance (the old formula used
	# the deceleration-only time over the whole run and took ~10s).
	var helper := LinearSnapHelper.new()
	var rv := await _make_rv(1920, 300, 100, helper)

	_press(rv, Vector2(1800, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(120, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(60, 100), true)
	await get_tree().process_frame
	_release(rv, Vector2(60, 100))

	# Must reach IDLE well before the unbounded linear settle would finish.
	await _await_idle(rv, 400)
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	rv.free_items()
	rv.free()


func test_detach_disables_snap() -> void:
	var helper := PagerSnapHelper.new()
	var rv := await _make_rv(200, 200, 10, helper)
	helper.detach()

	rv.scroll_horizontally(50)
	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_release(rv, Vector2(100, 100))
	await get_tree().process_frame

	# No snap: the offset stays where it was scrolled.
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(50)
	rv.free_items()
	rv.free()
