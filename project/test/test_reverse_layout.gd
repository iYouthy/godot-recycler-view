# Tests for reverse_layout (Linear/Grid/Staggered bottom-aligned layout), the
# position-based scroll APIs (scroll_to_position / smooth_scroll_to_position),
# and the reverse-aware scroll bar thumb.

extends GdUnitTestSuite


class BarAdapter extends Adapter:
	var count: int = 0

	func _get_item_count() -> int:
		return count

	func _get_item_extent(_p: int) -> int:
		return 40

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(360, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


func _make_rv(count: int) -> Dictionary:
	# Headless starts with a 64x64 window; pin the design size so synthetic
	# push_input events map 1:1 onto the RecyclerView (see test_scroll_fling).
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(360, 600))
	var adapter := BarAdapter.new()
	adapter.count = count
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func _free(rv: RecyclerView) -> void:
	rv.free_items()
	rv.free()


func _find_holder(rv: RecyclerView, position: int):
	for i in rv.get_child_holder_count():
		var h: ViewHolder = rv.get_child_holder_at(i)
		if h.get_position() == position:
			return h
	return null


func test_linear_normal_places_first_item_at_top() -> void:
	var s := await _make_rv(10)
	var rv: RecyclerView = s.rv
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	await get_tree().process_frame
	var h0 = _find_holder(rv, 0)
	assert_that(h0).is_not_null()
	assert_that(int(h0.get_control().position.y)).is_equal(0)
	_free(rv)


func test_linear_reverse_places_first_item_at_bottom() -> void:
	var s := await _make_rv(10)
	var rv: RecyclerView = s.rv
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	# 10 * 40 = 400 content in a 600 viewport: item 0 bottom-aligned at 600-40.
	var h0 = _find_holder(rv, 0)
	assert_that(h0).is_not_null()
	assert_that(int(h0.get_control().position.y)).is_equal(560)
	_free(rv)


func test_reverse_scroll_shows_later_items() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	# offset 0 shows the start (item 0 at the bottom).
	assert_that(_find_holder(rv, 0)).is_not_null()
	# Scroll to the end (max = 4000 - 600): the last item becomes visible.
	rv.set_scroll_offset(3400)
	await get_tree().process_frame
	assert_that(_find_holder(rv, 99)).is_not_null()
	assert_that(_find_holder(rv, 0)).is_null()
	_free(rv)


func test_grid_reverse_places_last_row_at_bottom() -> void:
	var s := await _make_rv(10)
	var rv: RecyclerView = s.rv
	var layout := GridLayoutManager.new()
	layout.set_span_count(3)
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	# 10 items / 3 cols = 4 rows of 40. Row 3 offset = 120.
	var h9 = _find_holder(rv, 9)
	assert_that(h9).is_not_null()
	# reverse: y = viewport - (row_offset + row_height) = 600 - (120 + 40) = 440.
	assert_that(int(h9.get_control().position.y)).is_equal(440)
	_free(rv)


func test_staggered_reverse_places_first_item_at_bottom() -> void:
	var s := await _make_rv(5)
	var rv: RecyclerView = s.rv
	var layout := StaggeredGridLayoutManager.new()
	layout.set_span_count(2)
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	var h0 = _find_holder(rv, 0)
	assert_that(h0).is_not_null()
	# position 0 in column 0, top 0, height 40 -> y = 600 - (0 + 40) = 560.
	assert_that(int(h0.get_control().position.y)).is_equal(560)
	_free(rv)


func test_scroll_to_position_normal_top_aligns() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	await get_tree().process_frame
	rv.scroll_to_position(50)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(2000)  # 50 * 40
	var h50 = _find_holder(rv, 50)
	assert_that(h50).is_not_null()
	assert_that(int(h50.get_control().position.y)).is_equal(0)
	_free(rv)


func test_scroll_to_position_reverse_bottom_aligns() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	rv.scroll_to_position(50)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(2000)
	var h50 = _find_holder(rv, 50)
	assert_that(h50).is_not_null()
	# reverse: the item's bottom aligns the viewport bottom -> y = 600 - 40.
	assert_that(int(h50.get_control().position.y)).is_equal(560)
	_free(rv)


func test_smooth_scroll_to_position_reaches_target() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	await get_tree().process_frame
	rv.smooth_scroll_to_position(50, 0.1)
	var frames := 0
	while rv.get_scroll_offset() != 2000 and frames < 120:
		await get_tree().process_frame
		frames += 1
	assert_that(rv.get_scroll_offset()).is_equal(2000)
	_free(rv)


func test_scroll_bar_reverse_thumb_at_bottom() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.set_scroll_bar(DefaultScrollBar.new())
	rv.request_layout()
	await get_tree().process_frame
	var bar: DefaultScrollBar = rv.get_scroll_bar()
	var thumb: Rect2 = bar.get_thumb_rect()
	# offset 0 = content start; on screen that region is at the bottom, so the
	# thumb (reported from the content end) sits at the bottom of the track.
	assert_that(thumb.size.y).is_greater(0.0)
	assert_that(thumb.position.y).is_greater(thumb.size.y * 2)
	# Scrolling deeper moves the thumb up (mirrored thumb).
	rv.set_scroll_offset(3400)
	await get_tree().process_frame
	assert_that(bar.get_thumb_rect().position.y).is_less(thumb.position.y)
	_free(rv)


func _drag_up(rv: RecyclerView) -> void:
	# Finger drags upward 200px inside the RV (content should follow the finger).
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = Vector2(180, 300)
	get_tree().root.push_input(press)
	await get_tree().process_frame
	var motion := InputEventMouseMotion.new()
	motion.button_mask = MOUSE_BUTTON_MASK_LEFT
	motion.position = Vector2(180, 100)
	get_tree().root.push_input(motion)
	await get_tree().process_frame
	var release := InputEventMouseButton.new()
	release.button_index = MOUSE_BUTTON_LEFT
	release.pressed = false
	release.position = Vector2(180, 100)
	get_tree().root.push_input(release)
	await get_tree().process_frame


func test_reverse_drag_up_moves_content_up() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	rv.set_layout(layout)
	rv.request_layout()
	await get_tree().process_frame
	rv.set_scroll_offset(2000)
	await get_tree().process_frame
	var before := rv.get_scroll_offset()
	await _drag_up(rv)
	# reverse: content up = older items = offset decreases.
	assert_that(rv.get_scroll_offset()).is_less(before)
	_free(rv)


func test_normal_drag_up_moves_content_up() -> void:
	var s := await _make_rv(100)
	var rv: RecyclerView = s.rv
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	await get_tree().process_frame
	rv.set_scroll_offset(2000)
	await get_tree().process_frame
	var before := rv.get_scroll_offset()
	await _drag_up(rv)
	# normal: content up = deeper items = offset increases.
	assert_that(rv.get_scroll_offset()).is_greater(before)
	_free(rv)
