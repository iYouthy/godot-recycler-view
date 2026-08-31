# Tests for the ItemTouchHelper: long-press drag-reorder, horizontal swipe to
# dismiss, flag gating, and ItemAnimator exclusion of the occupied holder.

extends GdUnitTestSuite


class TouchAdapter extends Adapter:
	var items: Array = []
	var created := 0
	var swiped_pos := -1
	var swiped_dir := 0

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(items[position])

	func move_item(from: int, to: int) -> void:
		var v = items[from]
		items.remove_at(from)
		items.insert(to, v)
		notify_item_moved(from, to)

	func remove_item(pos: int) -> void:
		items.remove_at(pos)
		swiped_pos = pos
		notify_item_removed(pos)


class TouchCallback extends ItemTouchHelperCallback:
	var adapter: TouchAdapter
	var drag_flags := ItemTouchHelper.DOWN | ItemTouchHelper.UP
	var swipe_flags := ItemTouchHelper.LEFT
	var move_count := 0
	var swipe_count := 0
	var velocity_threshold_consulted := 0

	func _get_movement_flags(holder: ViewHolder) -> int:
		return ItemTouchHelper.make_movement_flags(drag_flags, swipe_flags)

	func _get_swipe_velocity_threshold(default: float) -> float:
		velocity_threshold_consulted += 1
		return default

	func _on_move(recycler_view, dragged: ViewHolder, target: ViewHolder) -> bool:
		move_count += 1
		adapter.move_item(dragged.get_position(), target.get_position())
		return true

	func _on_swiped(holder: ViewHolder, direction: int) -> void:
		swipe_count += 1
		adapter.swiped_dir = direction
		adapter.remove_item(holder.get_position())


func _make_setup(callback: TouchCallback = null) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := TouchAdapter.new()
	for i in 10:
		adapter.items.append(i)
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	if callback == null:
		callback = TouchCallback.new()
	callback.adapter = adapter
	var helper := ItemTouchHelper.new()
	helper.set_callback(callback)
	helper.attach_to_recycler_view(rv)
	return { "rv": rv, "adapter": adapter, "callback": callback, "helper": helper }


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


func _wait_drag(helper: ItemTouchHelper, max_frames: int = 200) -> bool:
	for i in max_frames:
		if helper.is_dragging():
			return true
		await get_tree().process_frame
	return helper.is_dragging()


func _wait_swiped(adapter: TouchAdapter, max_frames: int = 300) -> bool:
	for i in max_frames:
		if adapter.swiped_pos >= 0:
			return true
		await get_tree().process_frame
	return adapter.swiped_pos >= 0


# Long-press item 0 and drag it down past two items: the data is reordered via
# on_move + notify_item_moved and the dragged holder settles in its new slot.
func test_long_press_drag_reorders() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback
	var helper: ItemTouchHelper = s.helper
	helper.set_long_press_timeout(5)

	_press(rv, Vector2(100, 20))
	assert_that(await _wait_drag(helper)).is_true()
	assert_that(helper.get_action_state()).is_equal(ItemTouchHelper.ACTION_STATE_DRAG)
	assert_that(helper.get_selected_holder()).is_not_null()

	# Cross item 1, flush the swap layout, then cross item 2.
	_motion(rv, Vector2(100, 100), true)
	await get_tree().process_frame
	_motion(rv, Vector2(100, 120), true)
	await get_tree().process_frame
	_release(rv, Vector2(100, 120))
	await get_tree().process_frame

	assert_that(callback.move_count).is_equal(2)
	assert_that(adapter.items[0]).is_equal(1)
	assert_that(adapter.items[1]).is_equal(2)
	assert_that(adapter.items[2]).is_equal(0)
	rv.free_items()
	rv.free()


# A long-press on an item without drag flags never selects it (the RV keeps the
# gesture).
func test_long_press_without_drag_flag_does_not_drag() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var callback: TouchCallback = s.callback
	var helper: ItemTouchHelper = s.helper
	callback.drag_flags = 0
	helper.set_long_press_timeout(1)

	_press(rv, Vector2(100, 20))
	for i in 30:
		await get_tree().process_frame
	assert_that(helper.get_action_state()).is_equal(ItemTouchHelper.ACTION_STATE_IDLE)
	assert_that(helper.get_selected_holder()).is_null()
	_release(rv, Vector2(100, 20))
	await get_tree().process_frame
	rv.free_items()
	rv.free()


# Swiping left past half the viewport deletes the item: after the slide-out
# animation, on_swiped removes it from the data.
func test_swipe_left_deletes() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback

	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), true)  # dx = -180 > 100 (half the 200px viewport)
	await get_tree().process_frame
	_release(rv, Vector2(10, 20))

	assert_that(await _wait_swiped(adapter)).is_true()
	assert_that(callback.swipe_count).is_equal(1)
	assert_that(adapter.swiped_dir).is_equal(ItemTouchHelper.LEFT)
	assert_that(adapter.items.size()).is_equal(9)
	assert_that(adapter.items[0]).is_equal(1)  # item 0 removed
	rv.free_items()
	rv.free()


# The swipe commit consults _get_swipe_velocity_threshold: ItemTouchHelper
# clamps computed velocities to it (Android's
# VelocityTracker.computeCurrentVelocity maxVelocity / Callback.getSwipeVelocityThreshold)
# before comparing them against the escape velocity.
func test_swipe_consults_velocity_threshold() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback

	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(100, 20), true)  # 1st: below threshold, no selection
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), true)  # 2nd: past threshold, selects the swipe
	await get_tree().process_frame
	_motion(rv, Vector2(0, 20), true)  # 3rd: records a second velocity sample
	await get_tree().process_frame
	_release(rv, Vector2(0, 20))

	assert_that(await _wait_swiped(adapter)).is_true()
	assert_that(callback.velocity_threshold_consulted).is_greater_equal(1)
	rv.free_items()
	rv.free()


# A small swipe (below the threshold, no flick velocity) bounces back: the item
# stays and on_swiped is never called.
func test_small_swipe_cancels() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback

	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	# Both motions in the same frame keep the velocity sampler at dt=0 so the
	# flick check fails deterministically; |dx|=20 is far below the threshold.
	_motion(rv, Vector2(180, 20), true)
	_motion(rv, Vector2(170, 20), true)
	_release(rv, Vector2(170, 20))

	var frames := 0
	while callback.swipe_count > 0 and frames < 300:
		await get_tree().process_frame
		frames += 1
	assert_that(callback.swipe_count).is_equal(0)
	assert_that(adapter.swiped_pos).is_equal(-1)
	assert_that(adapter.items.size()).is_equal(10)
	# The item bounced back to its slot.
	await get_tree().process_frame
	var h0: ViewHolder = rv.get_child_holder_at(0)
	assert_that(int(h0.get_control().position.y)).is_equal(0)
	rv.free_items()
	rv.free()


# Without swipe flags a horizontal drag is a plain scroll gesture: the helper
# never selects a swipe.
func test_swipe_respects_flags() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var callback: TouchCallback = s.callback
	var helper: ItemTouchHelper = s.helper
	callback.swipe_flags = 0

	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), true)
	await get_tree().process_frame
	assert_that(helper.get_action_state()).is_equal(ItemTouchHelper.ACTION_STATE_IDLE)
	assert_that(helper.get_selected_holder()).is_null()
	_release(rv, Vector2(10, 20))
	await get_tree().process_frame
	rv.free_items()
	rv.free()


# With an ItemAnimator attached, the dragged holder is excluded from the move
# animation (is_occupied) and stays pinned under the finger after a swap layout.
func test_drag_skips_item_animator_move() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var helper: ItemTouchHelper = s.helper
	rv.set_item_animator(DefaultItemAnimator.new())
	helper.set_long_press_timeout(5)
	await get_tree().process_frame

	_press(rv, Vector2(100, 20))
	assert_that(await _wait_drag(helper)).is_true()
	var dragged: ViewHolder = helper.get_selected_holder()
	assert_that(rv.is_item_touch_occupied(dragged)).is_true()

	_motion(rv, Vector2(100, 100), true)  # swap 0<->1
	await get_tree().process_frame
	assert_that(rv.get_item_animator().is_animating(dragged)).is_false()
	# Pinned at m_selected_start(0) + dy(80), not slid to its new slot.
	assert_that(dragged.get_control().position.y).is_equal(80.0)

	_release(rv, Vector2(100, 100))
	await get_tree().process_frame
	rv.free_items()
	rv.free()


# attach -> swipe deletes; detach -> the same gesture is an ordinary RV drag and
# nothing is deleted.
func test_attach_detach() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var helper: ItemTouchHelper = s.helper

	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), true)
	_release(rv, Vector2(10, 20))
	assert_that(await _wait_swiped(adapter)).is_true()
	assert_that(adapter.items.size()).is_equal(9)

	helper.detach()
	assert_that(rv.get_item_touch_helper()).is_null()
	_press(rv, Vector2(190, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), true)
	_release(rv, Vector2(10, 20))
	await get_tree().process_frame
	assert_that(adapter.items.size()).is_equal(9)
	assert_that(helper.get_action_state()).is_equal(ItemTouchHelper.ACTION_STATE_IDLE)
	rv.free_items()
	rv.free()


# Merely hovering over the list (no button) must never select a swipe or
# dismiss an item: Godot delivers motions without a press, and a stale press
# origin would otherwise fake a huge delta.
func test_hover_does_not_select_swipe() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback
	var helper: ItemTouchHelper = s.helper

	_motion(rv, Vector2(10, 20), false)
	await get_tree().process_frame
	_motion(rv, Vector2(190, 20), false)
	await get_tree().process_frame
	_motion(rv, Vector2(10, 20), false)
	await get_tree().process_frame

	assert_that(helper.get_action_state()).is_equal(ItemTouchHelper.ACTION_STATE_IDLE)
	assert_that(helper.get_selected_holder()).is_null()
	assert_that(callback.swipe_count).is_equal(0)
	assert_that(adapter.items.size()).is_equal(10)
	rv.free_items()
	rv.free()


# Swiping right (when the flags allow it) dismisses the item just like left.
func test_swipe_right_deletes() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var callback: TouchCallback = s.callback
	callback.swipe_flags = ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT

	_press(rv, Vector2(10, 20))
	await get_tree().process_frame
	_motion(rv, Vector2(190, 20), true)
	await get_tree().process_frame
	_release(rv, Vector2(190, 20))

	assert_that(await _wait_swiped(adapter)).is_true()
	assert_that(adapter.swiped_dir).is_equal(ItemTouchHelper.RIGHT)
	assert_that(adapter.items.size()).is_equal(9)
	rv.free_items()
	rv.free()


# After a swipe-remove the remaining items animate up; pressing their slots
# again must still start a swipe. Hit-testing uses layout slots, so the
# mid-animation position of a sliding item never makes the press miss.
func test_repeat_swipes_with_animator() -> void:
	var s = await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: TouchAdapter = s.adapter
	var helper: ItemTouchHelper = s.helper
	rv.set_item_animator(DefaultItemAnimator.new())
	await get_tree().process_frame

	for k in 4:
		_press(rv, Vector2(190, 20))
		await get_tree().process_frame
		_motion(rv, Vector2(10, 20), true)
		await get_tree().process_frame
		_release(rv, Vector2(10, 20))
		var before := adapter.items.size()
		for i in 300:
			if adapter.items.size() < before:
				break
			await get_tree().process_frame
		for i in 2:
			await get_tree().process_frame

	assert_that(adapter.items.size()).is_equal(6)
	assert_that(adapter.swiped_pos).is_equal(0)
	rv.free_items()
	rv.free()
