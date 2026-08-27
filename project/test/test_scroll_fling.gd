# Tests for the scroll state machine: IDLE/DRAGGING/SETTLING transitions,
# fling inertia on release, scroll listener dispatch, and interruption.

extends GdUnitTestSuite


class PlainAdapter extends Adapter:
	var count: int = 0

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		vh.set_control(Control.new())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


class LoggingListener extends ScrollListener:
	var states: Array[int] = []
	var scrolled: Array[Vector2i] = []
	var total_dx := 0
	var total_dy := 0

	func _on_scroll_state_changed(state: int) -> void:
		states.append(state)

	func _on_scrolled(dx: int, dy: int) -> void:
		scrolled.append(Vector2i(dx, dy))
		total_dx += dx
		total_dy += dy


func _make_setup() -> Dictionary:
	# Headless starts with a 64x64 window and a mismatched content scale, which
	# breaks hit-testing and stretches synthetic event coordinates. Pin both to
	# the design size so pushed events map 1:1 onto the RecyclerView.
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := PlainAdapter.new()
	adapter.count = 100
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


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


# Drags straight down through several frames and releases, which launches a
# fling toward the top (pre-scroll must leave room for it).
func _drag_down_and_release(rv: RecyclerView) -> void:
	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(100, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(100, 250), true)
	await get_tree().process_frame
	_motion(rv, Vector2(100, 400), true)
	await get_tree().process_frame
	_release(rv, Vector2(100, 400))


func test_press_then_release_without_movement_cycles_states() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_DRAGGING)

	_release(rv, Vector2(100, 100))
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	assert_that(listener.states).is_equal([
		RecyclerView.SCROLL_STATE_DRAGGING,
		RecyclerView.SCROLL_STATE_IDLE,
	])
	assert_that(listener.scrolled).is_empty()  # a click never scrolls
	rv.free_items()
	rv.free()


func test_drag_dispatches_on_scrolled() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	rv.scroll_vertically(500)
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(100, 300), true)
	await get_tree().process_frame

	# offset = drag_start(500) - dy(200) = 300, actual scroll is -200.
	assert_that(rv.get_scroll_offset()).is_equal(300)
	assert_that(listener.scrolled).is_equal([Vector2i(0, -200)])
	assert_that(listener.total_dy).is_equal(-200)

	_release(rv, Vector2(100, 300))
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	rv.free_items()
	rv.free()


func test_release_with_velocity_flings_and_settles() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	rv.scroll_vertically(500)
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	await _drag_down_and_release(rv)
	await get_tree().process_frame

	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_SETTLING)
	var offset_after_drag := rv.get_scroll_offset()

	var frames := 0
	while rv.get_scroll_state() != RecyclerView.SCROLL_STATE_IDLE and frames < 2000:
		await get_tree().process_frame
		frames += 1
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	# The fling glides toward the top, past the drag end position, never below 0.
	assert_that(rv.get_scroll_offset()).is_less(offset_after_drag)
	assert_that(rv.get_scroll_offset()).is_greater_equal(0)
	assert_that(listener.total_dy).is_less(0)
	rv.free_items()
	rv.free()


func test_press_during_fling_interrupts_it() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	rv.scroll_vertically(500)
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	await _drag_down_and_release(rv)
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_SETTLING)

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_DRAGGING)
	var parked := rv.get_scroll_offset()
	for i in 5:
		await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(parked)  # frozen, no fling drive

	_release(rv, Vector2(100, 100))
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	rv.free_items()
	rv.free()


func test_stop_scroll_aborts_fling() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	rv.scroll_vertically(500)
	await get_tree().process_frame

	await _drag_down_and_release(rv)
	await get_tree().process_frame
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_SETTLING)

	rv.stop_scroll()
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	var parked := rv.get_scroll_offset()
	for i in 5:
		await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(parked)
	rv.free_items()
	rv.free()


func test_wheel_scrolls_without_state_change() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	var wheel := InputEventMouseButton.new()
	wheel.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel.pressed = true
	wheel.position = Vector2(100, 100)
	get_tree().root.push_input(wheel)

	assert_that(rv.get_scroll_offset()).is_equal(48)
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	assert_that(listener.total_dy).is_equal(48)
	assert_that(listener.states).is_empty()  # wheel never transitions state
	rv.free_items()
	rv.free()


func test_remove_and_clear_listeners() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	rv.remove_on_scroll_listener(listener)
	rv.scroll_vertically(48)
	assert_that(listener.total_dy).is_equal(0)

	var other := LoggingListener.new()
	rv.add_on_scroll_listener(other)
	rv.clear_on_scroll_listeners()
	rv.scroll_vertically(48)
	assert_that(other.total_dy).is_equal(0)
	rv.free_items()
	rv.free()


func test_horizontal_drag_flings_along_x() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(640, 360))
	var adapter := PlainAdapter.new()
	adapter.count = 100
	rv.set_item_size(80)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame

	rv.scroll_horizontally(500)
	var listener := LoggingListener.new()
	rv.add_on_scroll_listener(listener)
	await get_tree().process_frame

	_press(rv, Vector2(100, 100))
	await get_tree().process_frame
	_motion(rv, Vector2(100, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(250, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(400, 100), true)
	await get_tree().process_frame
	_release(rv, Vector2(400, 100))
	await get_tree().process_frame

	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_SETTLING)
	var offset_after_drag := rv.get_scroll_offset_horizontal()

	var frames := 0
	while rv.get_scroll_state() != RecyclerView.SCROLL_STATE_IDLE and frames < 2000:
		await get_tree().process_frame
		frames += 1
	assert_that(rv.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	assert_that(rv.get_scroll_offset_horizontal()).is_less(offset_after_drag)
	assert_that(listener.total_dx).is_less(0)
	rv.free_items()
	rv.free()
