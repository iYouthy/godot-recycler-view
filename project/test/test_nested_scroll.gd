# Nested RecyclerView scrolling: wheel axis ownership, same-direction relay,
# and drag handoff between a child RV and its ancestor RV.

extends GdUnitTestSuite

const DividerDecoration := preload("res://divider_decoration.gd")


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


func _pin_window() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame


# Vertical parent with room to scroll (30 * 60 = 1800 content, 1200 max).
func _make_vertical_parent() -> RecyclerView:
	await _pin_window()
	var parent := RecyclerView.new()
	parent.position = Vector2(0, 0)
	parent.set_size(Vector2(300, 600))
	var adapter := PlainAdapter.new()
	adapter.count = 30
	parent.set_item_extent(60)
	parent.set_adapter(adapter)
	parent.set_layout(LinearLayoutManager.new())
	parent.request_layout()
	get_tree().root.add_child(parent)
	await get_tree().process_frame
	return parent


# Adds a child RV inside the parent's rect (as a topmost sibling Control, so it
# receives input in its area and its ancestor chain runs through the parent).
func _add_child_rv(parent: RecyclerView, pos: Vector2, size: Vector2, count: int, item_extent: int, horizontal: bool) -> RecyclerView:
	var child := RecyclerView.new()
	child.position = pos
	child.set_size(size)
	child.set_item_extent(item_extent)
	var adapter := PlainAdapter.new()
	adapter.count = count
	child.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	if horizontal:
		layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	child.set_layout(layout)
	child.request_layout()
	parent.add_child(child)
	await get_tree().process_frame
	return child


func _wheel(pos: Vector2, button: int) -> void:
	var w := InputEventMouseButton.new()
	w.button_index = button
	w.pressed = true
	w.position = pos
	get_tree().root.push_input(w)


func _press(pos: Vector2) -> void:
	var p := InputEventMouseButton.new()
	p.button_index = MOUSE_BUTTON_LEFT
	p.pressed = true
	p.position = pos
	get_tree().root.push_input(p)


func _release(pos: Vector2) -> void:
	var r := InputEventMouseButton.new()
	r.button_index = MOUSE_BUTTON_LEFT
	r.pressed = false
	r.position = pos
	get_tree().root.push_input(r)


func _motion(pos: Vector2) -> void:
	var m := InputEventMouseMotion.new()
	m.button_mask = MOUSE_BUTTON_MASK_LEFT
	m.position = pos
	get_tree().root.push_input(m)


# --- wheel nesting ---

func test_vertical_wheel_over_horizontal_child_scrolls_parent() -> void:
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 60), 20, 40, true)
	parent.scroll_vertically(300)
	await get_tree().process_frame

	# Vertical wheel over the horizontal child: it belongs to the vertical
	# parent, so the child must not map it to its own axis.
	_wheel(Vector2(100, 120), MOUSE_BUTTON_WHEEL_DOWN)
	assert_that(parent.get_scroll_offset()).is_equal(348)
	assert_that(child.get_scroll_offset_horizontal()).is_equal(0)

	child.free_items()
	parent.free_items()
	parent.free()


func test_same_direction_wheel_relays_leftover_to_parent() -> void:
	var parent := await _make_vertical_parent()
	# Vertical child (200x100, 5 items * 60 = 300 content, max 200).
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	child.scroll_vertically(180)
	await get_tree().process_frame
	assert_that(child.get_scroll_offset()).is_equal(180)

	# 48px wheel: the child consumes 20 to its max, the remaining 28 relays up.
	_wheel(Vector2(100, 150), MOUSE_BUTTON_WHEEL_DOWN)
	assert_that(child.get_scroll_offset()).is_equal(200)
	assert_that(parent.get_scroll_offset()).is_equal(28)

	child.free_items()
	parent.free_items()
	parent.free()


func test_three_level_wheel_relay() -> void:
	var grandparent := await _make_vertical_parent()
	var parent := await _add_child_rv(grandparent, Vector2(0, 0), Vector2(200, 150), 5, 60, false)
	var child := await _add_child_rv(parent, Vector2(0, 0), Vector2(100, 50), 3, 60, false)
	parent.scroll_vertically(140)
	child.scroll_vertically(120)
	await get_tree().process_frame

	_wheel(Vector2(50, 20), MOUSE_BUTTON_WHEEL_DOWN)
	# Child consumes 10 (120->130), parent consumes 10 (140->150), grandparent 28.
	assert_that(child.get_scroll_offset()).is_equal(130)
	assert_that(parent.get_scroll_offset()).is_equal(150)
	assert_that(grandparent.get_scroll_offset()).is_equal(28)

	child.free_items()
	parent.free_items()
	grandparent.free_items()
	grandparent.free()


func test_standalone_horizontal_wheel_still_maps_to_axis() -> void:
	# Regression: without a vertical ancestor the horizontal RV keeps mapping
	# the vertical wheel onto its own axis.
	await _pin_window()
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(300, 100))
	var adapter := PlainAdapter.new()
	adapter.count = 20
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame

	_wheel(Vector2(100, 50), MOUSE_BUTTON_WHEEL_DOWN)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(48)

	rv.free_items()
	rv.free()


# --- drag handoff (perpendicular) ---

func test_vertical_drag_over_horizontal_child_hands_off_to_parent() -> void:
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 60), 20, 40, true)
	parent.scroll_vertically(300)
	await get_tree().process_frame

	# Press inside the child; the first slop-crossing motion is vertical, so the
	# child hands the drag to the vertical parent.
	_press(Vector2(100, 120))
	await get_tree().process_frame
	_motion(Vector2(100, 140))  # still inside the child -> handoff happens here
	await get_tree().process_frame
	assert_that(parent.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_DRAGGING)
	assert_that(child.get_scroll_offset_horizontal()).is_equal(0)

	# Dragging further down (now over the parent) continues the parent's drag.
	_motion(Vector2(100, 240))
	await get_tree().process_frame
	assert_that(parent.get_scroll_offset()).is_equal(200)  # 300 - (240-140)
	assert_that(child.get_scroll_offset_horizontal()).is_equal(0)

	_release(Vector2(100, 240))
	await get_tree().process_frame
	assert_that(parent.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)

	child.free_items()
	parent.free_items()
	parent.free()


func test_horizontal_drag_over_horizontal_child_scrolls_child() -> void:
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 60), 20, 40, true)
	child.scroll_horizontally(200)
	parent.scroll_vertically(100)
	await get_tree().process_frame

	_press(Vector2(100, 120))
	await get_tree().process_frame
	_motion(Vector2(140, 120))  # dominant horizontal -> the child owns it
	await get_tree().process_frame
	assert_that(child.get_scroll_offset_horizontal()).is_equal(160)  # 200 - 40
	assert_that(parent.get_scroll_offset()).is_equal(100)

	_release(Vector2(140, 120))
	await get_tree().process_frame

	child.free_items()
	parent.free_items()
	parent.free()


# --- same-direction relay (drag + fling) ---

func test_same_direction_drag_relays_leftover_to_parent() -> void:
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	child.scroll_vertically(180)
	await get_tree().process_frame

	# Drag up (dy -40): the child scrolls 20 to its max and relays the other 20.
	_press(Vector2(100, 150))
	await get_tree().process_frame
	_motion(Vector2(100, 110))
	await get_tree().process_frame
	assert_that(child.get_scroll_offset()).is_equal(200)
	assert_that(parent.get_scroll_offset()).is_equal(20)

	_release(Vector2(100, 110))
	await get_tree().process_frame

	child.free_items()
	parent.free_items()
	parent.free()


func test_drag_leaving_child_and_returning_does_not_orphan() -> void:
	# Regression: Godot re-hit-tests every mouse event, so a drag owned by the
	# child RV would be orphaned once the mouse leaves its bounds (the release
	# never reaches it). Ancestors must route the gesture back to the grabber.
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	child.scroll_vertically(180)
	parent.scroll_vertically(50)
	await get_tree().process_frame

	# Drag up inside the child: it clamps at its max and spills 20 into parent.
	_press(Vector2(100, 150))
	await get_tree().process_frame
	_motion(Vector2(100, 110))
	await get_tree().process_frame
	assert_that(child.get_scroll_offset()).is_equal(200)
	var parent_after_inner := parent.get_scroll_offset()

	# Keep dragging up over the parent's area: the parent routes it to the child
	# (the grabber), so the child keeps spilling into the parent.
	_motion(Vector2(100, 60))
	await get_tree().process_frame
	assert_that(parent.get_scroll_offset()).is_greater(parent_after_inner)
	assert_that(child.get_scroll_offset()).is_equal(200)

	# Release outside the child: the child must leave DRAGGING (not be stuck).
	_release(Vector2(100, 60))
	await get_tree().process_frame
	assert_that(child.get_scroll_state()).is_not_equal(RecyclerView.SCROLL_STATE_DRAGGING)
	parent.stop_scroll()
	child.stop_scroll()
	await get_tree().process_frame
	var parked := child.get_scroll_offset()

	# Mouse returns over the child with the button up: no stale drag, no slide.
	_motion(Vector2(100, 140))
	await get_tree().process_frame
	_motion(Vector2(100, 170))
	await get_tree().process_frame
	assert_that(child.get_scroll_offset()).is_equal(parked)

	child.free_items()
	parent.free_items()
	parent.free()


func test_crossing_child_boundary_does_not_jump() -> void:
	# Regression: Godot transforms event positions into the receiving control's
	# local space, so a drag forwarded between nested RVs must convert the
	# position back into the child's space, or the offset jumps at the edge.
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var parent := RecyclerView.new()
	parent.position = Vector2(0, 0)
	parent.set_size(Vector2(360, 600))
	var adapter := DemoAdapter.new()
	adapter.count = 6
	parent.set_item_extent(48)
	parent.set_adapter(adapter)
	parent.set_layout(LinearLayoutManager.new())
	parent.add_item_decoration(DividerDecoration.new())
	parent.request_layout()
	get_tree().root.add_child(parent)
	await get_tree().process_frame

	var sub: RecyclerView = adapter.sub_rv
	sub.scroll_vertically(324)
	await get_tree().process_frame

	_press(Vector2(100, 150))  # inside the sub
	await get_tree().process_frame
	_motion(Vector2(100, 170))  # inside
	await get_tree().process_frame
	_motion(Vector2(100, 230))  # outside: forwarded by the parent grabber
	await get_tree().process_frame
	# The drag moved 80px (150 -> 230); the offset must follow exactly.
	assert_that(sub.get_scroll_offset()).is_equal(324 - 80)

	adapter.sub_rv = null  # free_items deletes the sub; drop the dangling ref
	parent.free_items()
	parent.free()


func test_drag_over_sibling_rv_keeps_flowing() -> void:
	# Regression: dragging a child RV and crossing into a sibling RV (e.g. a
	# horizontal chip row) must still route the motion to the grabbing child.
	var parent := await _make_vertical_parent()
	var sub := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	var sibling := await _add_child_rv(parent, Vector2(50, 0), Vector2(200, 60), 20, 40, true)
	sub.scroll_vertically(100)
	await get_tree().process_frame

	_press(Vector2(100, 150))  # inside the sub (y 100..200)
	await get_tree().process_frame
	_motion(Vector2(100, 130))
	await get_tree().process_frame
	_motion(Vector2(100, 110))
	await get_tree().process_frame
	var before := sub.get_scroll_offset()

	_motion(Vector2(100, 50))  # over the sibling RV above the sub
	await get_tree().process_frame
	assert_that(sub.get_scroll_offset()).is_greater(before)

	_release(Vector2(100, 50))
	await get_tree().process_frame
	parent.stop_scroll()
	sub.stop_scroll()

	sub.free_items()
	sibling.free_items()
	parent.free_items()
	parent.free()


class DemoAdapter extends Adapter:
	var count: int = 0
	var sub_rv: RecyclerView
	var sub_height: int = 120

	func _get_item_count() -> int:
		return count

	func _get_item_view_type(position: int) -> int:
		if position == 1:
			return 1
		return 0

	func _get_item_extent(position: int) -> int:
		if position == 1:
			return sub_height
		return 48

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		if view_type == 1:
			var root := Control.new()
			root.clip_contents = true
			root.set_size(Vector2(360, sub_height))
			var sub := RecyclerView.new()
			sub.set_size(Vector2(360, sub_height))
			sub.set_item_extent(36)
			var a := PlainAdapter.new()
			a.count = 15
			sub.set_adapter(a)
			sub.set_layout(LinearLayoutManager.new())
			sub.request_layout()
			root.add_child(sub)
			sub_rv = sub
			vh.set_control(root)
		else:
			var label := Label.new()
			label.set_size(Vector2(360, 48))
			vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


func test_interrupted_drag_does_not_fling() -> void:
	# Regression: when the release happens outside the window, only a later
	# motion with the button up reveals it. That detection must cancel the drag
	# without flinging on the stale velocity (neither this RV nor the ancestor).
	var parent := await _make_vertical_parent()
	var sub := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	sub.scroll_vertically(100)
	await get_tree().process_frame

	# Fast upward drag inside the sub (stores a high velocity).
	_press(Vector2(100, 150))
	await get_tree().process_frame
	_motion(Vector2(100, 130))
	await get_tree().process_frame
	_motion(Vector2(100, 110))
	await get_tree().process_frame
	var parked := sub.get_scroll_offset()

	# The release happened outside the window; the mouse re-entering delivers a
	# button-up motion, which must cancel the drag, not start a stale fling.
	var enter := InputEventMouseMotion.new()
	enter.button_mask = 0
	enter.position = Vector2(100, 110)
	get_tree().root.push_input(enter)
	await get_tree().process_frame
	assert_that(sub.get_scroll_state()).is_not_equal(RecyclerView.SCROLL_STATE_SETTLING)
	assert_that(parent.get_scroll_state()).is_not_equal(RecyclerView.SCROLL_STATE_SETTLING)

	for i in 10:
		await get_tree().process_frame
	assert_that(sub.get_scroll_offset()).is_equal(parked)
	assert_that(parent.get_scroll_offset()).is_equal(0)

	sub.free_items()
	parent.free_items()
	parent.free()


func test_mid_list_fling_does_not_relay_to_parent() -> void:
	# A fling started mid-list must be consumed by the child: it scrolls toward
	# its own boundary, and the outer RV must not fly. (Headless frames are too
	# coarse to always produce the high velocity that clamps the fling, so this
	# guards the contract; the editor exercise covers the clamped case.)
	var parent := await _make_vertical_parent()
	var sub := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	sub.scroll_vertically(100)  # mid-list (max 200)
	await get_tree().process_frame

	_press(Vector2(100, 180))  # inside the sub (y 100..200)
	await get_tree().process_frame
	_motion(Vector2(100, 150))
	await get_tree().process_frame
	_motion(Vector2(100, 120))
	await get_tree().process_frame
	_release(Vector2(100, 120))
	await get_tree().process_frame

	var frames := 0
	while sub.get_scroll_state() != RecyclerView.SCROLL_STATE_IDLE and frames < 2000:
		await get_tree().process_frame
		frames += 1
	assert_that(parent.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	assert_that(parent.get_scroll_offset()).is_equal(0)

	sub.free_items()
	parent.free_items()
	parent.free()


func test_same_direction_fling_relays_to_parent() -> void:
	var parent := await _make_vertical_parent()
	var child := await _add_child_rv(parent, Vector2(50, 100), Vector2(200, 100), 5, 60, false)
	child.scroll_vertically(200)  # child pinned at its bottom
	await get_tree().process_frame

	# A quick upward drag inside the child: every motion spills into the parent
	# (already at max), and the release velocity starts a parent fling.
	_press(Vector2(100, 150))
	await get_tree().process_frame
	_motion(Vector2(100, 130))
	await get_tree().process_frame
	_motion(Vector2(100, 110))
	await get_tree().process_frame
	_release(Vector2(100, 110))
	await get_tree().process_frame

	assert_that(child.get_scroll_offset()).is_equal(200)
	assert_that(parent.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_SETTLING)

	var frames := 0
	while parent.get_scroll_state() != RecyclerView.SCROLL_STATE_IDLE and frames < 2000:
		await get_tree().process_frame
		frames += 1
	assert_that(parent.get_scroll_state()).is_equal(RecyclerView.SCROLL_STATE_IDLE)
	# The parent carried the momentum past the drag's spill.
	assert_that(parent.get_scroll_offset()).is_greater(0)

	child.free_items()
	parent.free_items()
	parent.free()
