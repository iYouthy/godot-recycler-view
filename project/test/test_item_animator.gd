# Tests for ItemAnimator + DefaultItemAnimator: move/add/remove/change
# animations driven by the two-phase layout after incremental updates.

extends GdUnitTestSuite


class ValueAdapter extends Adapter:
	var items: Array = []

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = str(items[position])


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := ValueAdapter.new()
	adapter.items = [0, 1, 2, 3, 4]
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.set_item_animator(DefaultItemAnimator.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


# Waits until the animator reports no running animation (bounded).
func _await_idle(rv: RecyclerView) -> void:
	var animator: ItemAnimator = rv.get_item_animator()
	var frames := 0
	while animator.is_running() and frames < 2000:
		await get_tree().process_frame
		frames += 1


# New holders are appended to the child list, so locate items by their label
# text instead of by index.
func _holder_by_text(rv: RecyclerView, text: String) -> ViewHolder:
	for i in rv.get_child_holder_count():
		var c: Control = rv.get_child_holder_at(i).get_control()
		if c is Label and (c as Label).text == text:
			return rv.get_child_holder_at(i)
	return null


func test_set_and_get_animator() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	assert_that(rv.get_item_animator()).is_not_null()
	rv.free_items()
	rv.free()


func test_insert_animates_existing_items_down() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	assert_that(animator.is_running()).is_true()

	# Item "0" slides from its pre position (0) toward 40.
	var moving: Control = _holder_by_text(rv, "0").get_control()
	var moved_somewhere := false
	for i in 5:
		await get_tree().process_frame
		if moving.position.y > 0.0:
			moved_somewhere = true
	assert_that(moved_somewhere).is_true()

	await _await_idle(rv)
	assert_that(int(moving.position.y)).is_equal(40)
	assert_that(animator.is_running()).is_false()
	rv.free_items()
	rv.free()


func test_insert_animates_new_item_fade_in() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	# The new item starts transparent (animation may already be a few frames in).
	var added: Control = _holder_by_text(rv, "9").get_control()
	assert_that(added.modulate.a).is_less(0.05)

	await _await_idle(rv)
	assert_that(added.modulate.a).is_greater(0.99)
	rv.free_items()
	rv.free()


func test_remove_fades_out_then_recycles() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	var doomed: Control = _holder_by_text(rv, "0").get_control()
	adapter.items = [1, 2, 3, 4]
	rv.notify_item_range_removed(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	assert_that(animator.is_running()).is_true()

	# The removed control fades out but stays in the tree during the animation.
	assert_that(doomed.get_parent()).is_not_null()
	await _await_idle(rv)
	# After the animation it is detached (recycled).
	assert_that(doomed.get_parent()).is_null()
	# The remaining items settled: item "1" is now first.
	var first: Control = _holder_by_text(rv, "1").get_control()
	assert_that(int(first.position.y)).is_equal(0)
	rv.free_items()
	rv.free()


func test_animating_holder_not_recycled_on_scroll() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	var held: ViewHolder = _holder_by_text(rv, "0")

	# Scroll while the move animation is running: the animating holder must not
	# be recycled (the animator keeps it alive until the animation finishes).
	rv.scroll_vertically(200)
	await get_tree().process_frame
	assert_that(animator.is_running()).is_true()
	assert_that(held.get_control().get_parent()).is_not_null()

	await _await_idle(rv)
	rv.free_items()
	rv.free()


func test_change_pulses_opacity() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	adapter.items = [0, 1, 5, 3, 4]
	rv.notify_item_range_changed(2, 1, null)
	await get_tree().process_frame  # notify defers the layout to end of frame
	assert_that(animator.is_running()).is_true()
	var changed: Control = _holder_by_text(rv, "5").get_control()

	var dipped := false
	for i in 10:
		await get_tree().process_frame
		if changed.modulate.a < 0.9:
			dipped = true
	assert_that(dipped).is_true()
	await _await_idle(rv)
	assert_that(changed.modulate.a).is_greater(0.99)
	rv.free_items()
	rv.free()


func test_grow_remove_then_rapid_inserts_do_not_crash() -> void:
	# Regression: growing past the viewport, deleting items, then rapidly adding
	# used to treat holders recycled out of range as removed, double-referencing
	# them (cache + animator) and crashing with no log. Animating holders must be
	# kept attached and only data-removed holders animated.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	var animator: ItemAnimator = rv.get_item_animator()

	for i in 20:
		adapter.items.insert(0, -i)
		rv.notify_item_range_inserted(0, 1)
		await _await_idle(rv)

	adapter.items.remove_at(2)
	rv.notify_item_range_removed(2, 1)
	await get_tree().process_frame
	adapter.items.remove_at(2)
	rv.notify_item_range_removed(2, 1)
	await get_tree().process_frame

	for i in 30:
		adapter.items.insert(0, -i - 100)
		rv.notify_item_range_inserted(0, 1)
		await get_tree().process_frame

	await _await_idle(rv)
	rv.request_layout()
	assert_that(animator.is_running()).is_false()
	# The visible window (600/40 = 15) plus a small margin.
	assert_that(rv.get_child_holder_count()).is_less_equal(20)
	rv.free_items()
	rv.free()


func test_move_starts_at_from_not_end() -> void:
	# Regression: the layout positions the item at its target, so a move must
	# snap it back to its start immediately. Otherwise the item renders at its
	# end for a frame and then "restarts", which reads as a jitter.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame

	var moving: Control = _holder_by_text(rv, "0").get_control()
	# It starts near its pre position (0) and is still far from the end (40).
	assert_that(int(moving.position.y)).is_less(40)
	rv.free_items()
	rv.free()


func test_rapid_followup_move_does_not_jump_to_end() -> void:
	# Regression: a second update arriving mid-move must continue from the
	# current position instead of rendering the item at the new end and then
	# replaying from the old start (two moves fighting the same control).
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # let the first move advance a little

	adapter.items = [8, 9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	var moving: Control = _holder_by_text(rv, "0").get_control()
	# Item "0" ends up at index 2 (target y=80) but must still be mid-flight,
	# not already snapped to the end.
	assert_that(int(moving.position.y)).is_less(80)
	rv.free_items()
	rv.free()


func test_overflow_head_insert_does_not_fly_tail_to_head() -> void:
	# Regression: once the list overflows the viewport, a head insert pushes the
	# tail holder out of view, and the same cycle's fill used to re-attach that
	# holder at position 0 (cache miss -> pool) while it still had a pre-update
	# position record — the dispatch then animated it from its old tail slot all
	# the way to the head ("the tail item flying to the top"). A holder recycled
	# out of range must sit the cycle out in the changed scrap instead.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	# 20 items overflow the 600px viewport (600 / 40 = 15 visible).
	for i in 15:
		adapter.items.append(i + 5)
	rv.request_layout()
	await get_tree().process_frame

	adapter.items.insert(0, 99)
	rv.notify_item_range_inserted(0, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame

	# Every visible holder must stay within one item of its slot for the whole
	# animation; a tail holder reused for the head would start a viewport away.
	var frames := 0
	while rv.get_item_animator().is_running() and frames < 2000:
		for i in rv.get_child_holder_count():
			var c: Control = rv.get_child_holder_at(i).get_control()
			var slot := rv.get_child_holder_at(i).get_position() * 40
			assert_that(absf(c.position.y - slot)).is_less_equal(40)
		await get_tree().process_frame
		frames += 1

	# And the new head item settled in place, fully opaque.
	var added: Control = _holder_by_text(rv, "99").get_control()
	assert_that(int(added.position.y)).is_equal(0)
	assert_that(added.modulate.a).is_greater(0.99)
	rv.free_items()
	rv.free()


func test_remove_after_move_fades_in_place() -> void:
	# Regression: an item that was mid-move when removed used to keep its move
	# animation running; the re-queried target for a removed holder (position is
	# NO_POSITION) resolved to (0,0) and dragged the fading control onto the
	# first item, overlapping it.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ValueAdapter = s.adapter
	adapter.items = [9, 0, 1, 2, 3, 4]
	rv.notify_item_range_inserted(0, 1)  # item "2" shifts down (move starts)
	await get_tree().process_frame
	var doomed: Control = _holder_by_text(rv, "2").get_control()

	adapter.items = [9, 0, 1, 3, 4]
	rv.notify_item_range_removed(3, 1)  # remove "2" mid-move
	# It must fade where it was (y=80), never drifting toward the top (y=0).
	var min_y := 1000
	for i in 10:
		await get_tree().process_frame
		if int(doomed.position.y) < min_y:
			min_y = int(doomed.position.y)
	assert_that(min_y).is_greater_equal(40)
	rv.free_items()
	rv.free()
