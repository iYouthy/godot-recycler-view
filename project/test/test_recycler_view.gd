# Tests for the vertical slice: RecyclerView hub + LinearLayoutManager + Recycler.

extends GdUnitTestSuite

const MultiTypeAdapter := preload("res://multi_type_adapter.gd")
const MixedAdapter := preload("res://mixed_adapter.gd")

class ItemAdapter extends Adapter:
	var count: int = 0
	var created: int = 0
	var bound: Array = []

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		vh.set_control(Control.new())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		bound.append(position)


func _make_setup() -> Dictionary:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := ItemAdapter.new()
	var layout := LinearLayoutManager.new()
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(layout)
	return { "rv": rv, "adapter": adapter }


func test_instantiates_only_visible_items() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	# 600 / 60 = 10 visible.
	assert_that(rv.get_child_holder_count()).is_equal(10)
	assert_that(adapter.created).is_equal(10)
	rv.free_items()
	rv.free()


func test_zero_or_negative_items() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 0
	rv.request_layout()
	assert_that(rv.get_child_holder_count()).is_equal(0)
	rv.free_items()
	rv.free()


func test_scroll_moves_visible_window_and_recycles() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	var created_before := adapter.created

	rv.scroll_vertically(60)
	# Still 10 visible, but position window shifted by one.
	assert_that(rv.get_child_holder_count()).is_equal(10)
	assert_that(rv.get_scroll_offset()).is_equal(60)
	# One new item came into view; one was recycled. Creation stays bounded.
	assert_that(adapter.created).is_equal(created_before + 1)
	rv.free_items()
	rv.free()


func test_scroll_repositions_remaining_holders() -> void:
	# Regression: scrolling must move the kept holders too, otherwise the new
	# items pile up at the same slot instead of following the scroll offset.
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()

	rv.scroll_vertically(60)
	# Window shifted from 0..9 to 1..10: pos 1 at y=0 ... pos 10 at y=540.
	assert_that(rv.get_child_holder_count()).is_equal(10)
	var ys: Array = []
	for i in rv.get_child_holder_count():
		var holder: ViewHolder = rv.get_child_holder_at(i)
		ys.append(holder.get_control().position.y)
	assert_that(ys.size()).is_equal(10)
	var unique_ys := {}
	for y in ys:
		unique_ys[y] = true
	# Every holder lands on a distinct slot (no stacking).
	assert_that(unique_ys.size()).is_equal(10)
	assert_that(ys.has(0.0)).is_true()
	assert_that(ys.has(540.0)).is_true()

	# And scrolling back down restores the original positions.
	rv.scroll_vertically(-60)
	var ys_back: Array = []
	for i in rv.get_child_holder_count():
		var holder: ViewHolder = rv.get_child_holder_at(i)
		ys_back.append(holder.get_control().position.y)
	assert_that(ys_back.has(0.0)).is_true()
	assert_that(ys_back.has(540.0)).is_true()
	rv.free_items()
	rv.free()


func test_scroll_through_all_items_bounds_creation() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 1000
	rv.request_layout()
	for i in 50:
		rv.scroll_vertically(60)
	# Without recycling this would create ~60 holders; with reuse it stays small.
	assert_that(adapter.created).is_less(20)
	assert_that(rv.get_child_holder_count()).is_equal(10)
	rv.free_items()
	rv.free()


func test_scroll_clamps_at_content_end() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 5
	rv.request_layout()
	rv.scroll_vertically(10000)
	# Content 5*60=300, viewport 600 -> no scroll possible.
	assert_that(rv.get_scroll_offset()).is_equal(0)
	assert_that(rv.get_child_holder_count()).is_equal(5)
	rv.free_items()
	rv.free()


func test_adapter_notify_triggers_relayout() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 10
	rv.request_layout()
	assert_that(rv.get_child_holder_count()).is_equal(10)

	adapter.count = 30
	adapter.notify_item_inserted(10)
	# Observer triggers a re-layout; the visible window is still 10 but content grew.
	assert_that(rv.get_child_holder_count()).is_equal(10)
	rv.scroll_vertically(10000)
	assert_that(rv.get_scroll_offset()).is_equal(1200)
	rv.free_items()
	rv.free()


func test_recycler_holds_cached_and_pooled() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ItemAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	rv.scroll_vertically(600)
	# After scrolling one viewport, old holders are in the cache/pool.
	assert_that(rv.get_recycler().size() > 0).is_true()
	rv.free_items()
	rv.free()


func test_release_outside_window_clears_drag() -> void:
	# Regression: releasing the left button outside the window never delivers a
	# release event, so a stale drag must be cleared by the button mask on the
	# next motion event. Otherwise re-entering the window without the button
	# held keeps scrolling the list with the cursor.
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := ItemAdapter.new()
	adapter.count = 100
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame

	rv.scroll_vertically(500)
	assert_that(rv.get_scroll_offset()).is_equal(500)

	# Press, then drag with the left button held: the offset follows the cursor.
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = Vector2(100, 100)
	get_tree().root.push_input(press)
	await get_tree().process_frame

	var drag := InputEventMouseMotion.new()
	drag.button_mask = MOUSE_BUTTON_MASK_LEFT
	drag.position = Vector2(100, 300)
	get_tree().root.push_input(drag)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(300)

	# Re-enter the window with the button up (release happened outside): the
	# drag must be cleared and the list must not move with the cursor.
	var enter_up := InputEventMouseMotion.new()
	enter_up.button_mask = 0
	enter_up.position = Vector2(100, 500)
	get_tree().root.push_input(enter_up)
	await get_tree().process_frame
	var offset_before := rv.get_scroll_offset()
	var move_up := InputEventMouseMotion.new()
	move_up.button_mask = 0
	move_up.position = Vector2(100, 600)
	get_tree().root.push_input(move_up)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(offset_before)

	# A fresh press + drag still works after the stale drag was cleared.
	var press2 := InputEventMouseButton.new()
	press2.button_index = MOUSE_BUTTON_LEFT
	press2.pressed = true
	press2.position = Vector2(100, 100)
	get_tree().root.push_input(press2)
	await get_tree().process_frame
	var drag2 := InputEventMouseMotion.new()
	drag2.button_mask = MOUSE_BUTTON_MASK_LEFT
	drag2.position = Vector2(100, 300)
	get_tree().root.push_input(drag2)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(100)

	rv.free_items()
	rv.free()


# Adapter backed by a mutable value list; binds the item's value into a Label.
class ValueAdapter extends Adapter:
	var items: Array = []
	var created: int = 0
	var bound: Array = []

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		vh.set_control(Label.new())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		bound.append(position)
		var label: Label = holder.get_control()
		label.text = str(items[position])


class DiffCallback extends DiffUtilCallback:
	var old_items: Array = []
	var new_items: Array = []

	func _get_old_list_size() -> int:
		return old_items.size()

	func _get_new_list_size() -> int:
		return new_items.size()

	func _are_items_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_items[old_item_position] == new_items[new_item_position]

	func _are_contents_the_same(old_item_position: int, new_item_position: int) -> bool:
		return true


func _make_value_setup() -> Dictionary:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := ValueAdapter.new()
	for i in 100:
		adapter.items.append(i)
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	return { "rv": rv, "adapter": adapter }


func _find_holder(rv: RecyclerView, position: int) -> ViewHolder:
	for i in rv.get_child_holder_count():
		var holder: ViewHolder = rv.get_child_holder_at(i)
		if holder.get_position() == position:
			return holder
	return null


func test_notify_insert_preserves_and_offsets_holders() -> void:
	var setup := _make_value_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ValueAdapter = setup.adapter
	rv.scroll_vertically(300)  # window shows positions 5..14
	var created_before := adapter.created

	# Insert above the viewport: existing holders shift +1, creation stays bounded.
	adapter.items.insert(3, -1)
	rv.notify_item_range_inserted(3, 1)
	assert_that(rv.get_child_holder_count()).is_equal(10)
	assert_that(adapter.created).is_less(created_before + 3)
	# The visible window now starts at item 4 (old index 3 was shifted out of view
	# is not shown; position 5 holds the value that was at index 4).
	var h5 := _find_holder(rv, 5)
	assert_that(h5).is_not_null()
	assert_that(h5.get_control().text).is_equal("4")
	var h6 := _find_holder(rv, 6)
	assert_that(h6).is_not_null()
	assert_that(h6.get_control().text).is_equal("5")
	rv.free_items()
	rv.free()


func test_notify_remove_reuses_holders() -> void:
	var setup := _make_value_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ValueAdapter = setup.adapter
	rv.scroll_vertically(300)
	var created_before := adapter.created

	adapter.items.remove_at(3)
	rv.notify_item_range_removed(3, 1)
	assert_that(rv.get_child_holder_count()).is_equal(10)
	# The window shifts by one slot; creation stays bounded (at most one new).
	assert_that(adapter.created).is_less(created_before + 3)
	# After the removal the window 5..14 shows items 5..14 of the new list.
	var h5 := _find_holder(rv, 5)
	assert_that(h5).is_not_null()
	assert_that(h5.get_control().text).is_equal("6")
	rv.free_items()
	rv.free()


func test_notify_move_offsets_holders() -> void:
	var setup := _make_value_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ValueAdapter = setup.adapter
	rv.scroll_vertically(300)
	var created_before := adapter.created

	# Move index 8 to index 3 (above the viewport): holders 5..8 shift.
	var value = adapter.items[8]
	adapter.items.remove_at(8)
	adapter.items.insert(3, value)
	rv.notify_item_moved(8, 3)
	assert_that(rv.get_child_holder_count()).is_equal(10)
	assert_that(adapter.created).is_less(created_before + 3)
	# After the move the window 5..14 shows items 5..14 of the new list.
	var h5 := _find_holder(rv, 5)
	assert_that(h5).is_not_null()
	assert_that(h5.get_control().text).is_equal("4")
	rv.free_items()
	rv.free()


func test_notify_change_rebinds_visible() -> void:
	var setup := _make_value_setup()
	var rv: RecyclerView = setup.rv
	var adapter: ValueAdapter = setup.adapter
	rv.scroll_vertically(300)

	adapter.bound.clear()
	adapter.items[8] = 999
	rv.notify_item_range_changed(8, 1, null)
	# The visible holder at position 8 is re-bound to the new content.
	var h8 := _find_holder(rv, 8)
	assert_that(h8).is_not_null()
	assert_that(h8.get_control().text).is_equal("999")
	assert_that(adapter.bound.has(8)).is_true()
	rv.free_items()
	rv.free()


func test_diff_dispatch_drives_incremental_update() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := ValueAdapter.new()
	for i in 20:
		adapter.items.append(i)
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	var created_before := adapter.created

	# New list: remove index 5 and append a fresh value.
	var old := adapter.items.duplicate()
	adapter.items.remove_at(5)
	adapter.items.append(999)
	var callback := DiffCallback.new()
	callback.old_items = old
	callback.new_items = adapter.items
	var diff := DiffUtil.calculate_diff(callback, true)
	var update := AdapterListUpdateCallback.new()
	update.set_adapter(adapter)
	diff.dispatch_updates_to(update)

	assert_that(rv.get_child_holder_count()).is_equal(10)
	# Creation stays bounded: the diff drove an incremental update, not a full rebuild.
	assert_that(adapter.created).is_less(created_before + 3)
	# The removed index 5 is gone; position 5 shows the item that followed it.
	var h5 := _find_holder(rv, 5)
	assert_that(h5).is_not_null()
	assert_that(h5.get_control().text).is_equal("6")
	rv.free_items()
	rv.free()


func test_multiple_view_types_reuse_and_match() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := MultiTypeAdapter.new()
	adapter.count = 1000
	rv.set_item_size(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	assert_that(rv.get_child_holder_count()).is_equal(10)

	# Every visible holder's view type must match its position's type.
	for i in rv.get_child_holder_count():
		var h: ViewHolder = rv.get_child_holder_at(i)
		assert_that(h.get_item_view_type()).is_equal(h.get_position() % 3)

	# Scroll far enough to cycle every type many times; types keep matching.
	for i in 80:
		rv.scroll_vertically(60)
		for j in rv.get_child_holder_count():
			var h: ViewHolder = rv.get_child_holder_at(j)
			assert_that(h.get_item_view_type()).is_equal(h.get_position() % 3)

	# Creation is bounded per type: reuse happens within each type's pool and the
	# position cache never hands a view of the wrong type to a position.
	assert_that(adapter.created.get(0, 0)).is_less(30)
	assert_that(adapter.created.get(1, 0)).is_less(30)
	assert_that(adapter.created.get(2, 0)).is_less(30)
	rv.free_items()
	rv.free()


func test_variable_item_heights_layout() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := MixedAdapter.new()
	for i in 10:
		if i % 2 == 0:
			adapter.items.append({ "type": "user", "name": "U%d" % i, "age": 20 })
		else:
			adapter.items.append({ "type": "message", "content": "M%d" % i, "is_read": false })
	rv.set_item_size(48)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()

	# Content height = 5 users * 72 + 5 messages * 48 = 600.
	var layout: LinearLayoutManager = rv.get_layout()
	assert_that(layout.get_content_size(rv)).is_equal(600)
	# Position 3 sits after user(0..72), message(72..120), user(120..192).
	assert_that(layout.get_item_offset(3)).is_equal(192)

	# Visible items are stacked by their own heights (no fixed stride).
	var expected_y := 0
	for i in rv.get_child_holder_count():
		var h: ViewHolder = rv.get_child_holder_at(i)
		var c: Control = h.get_control()
		assert_that(int(c.position.y)).is_equal(expected_y)
		assert_that(int(c.size.y)).is_equal(72 if h.get_position() % 2 == 0 else 48)
		expected_y += int(c.size.y)
	# All 10 items fit in the 600px viewport.
	assert_that(rv.get_child_holder_count()).is_equal(10)
	rv.free_items()
	rv.free()


func test_variable_heights_scroll_reuses_and_does_not_overlap() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := MixedAdapter.new()
	for i in 200:
		if i % 2 == 0:
			adapter.items.append({ "type": "user", "name": "U%d" % i, "age": 20 })
		else:
			adapter.items.append({ "type": "message", "content": "M%d" % i, "is_read": false })
	rv.set_item_size(48)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()

	for i in 40:
		rv.scroll_vertically(30)
		# No two visible items overlap vertically.
		var ys: Array = []
		for j in rv.get_child_holder_count():
			var c: Control = rv.get_child_holder_at(j).get_control()
			ys.append([c.position.y, c.position.y + c.size.y])
		ys.sort()
		for k in range(1, ys.size()):
			assert_that(ys[k][0] >= ys[k - 1][1] - 0.5).is_true()

	# Creation stays bounded per view type.
	assert_that(adapter.created.get(MixedAdapter.TYPE_USER, 0)).is_less(30)
	assert_that(adapter.created.get(MixedAdapter.TYPE_MESSAGE, 0)).is_less(30)
	rv.free_items()
	rv.free()


func test_horizontal_orientation_layouts_and_scrolls() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(640, 360))
	var adapter := ItemAdapter.new()
	adapter.count = 100
	rv.set_item_size(80)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.request_layout()

	# Items stack left-to-right, filling the viewport height.
	var expected_x := 0
	for i in rv.get_child_holder_count():
		var c: Control = rv.get_child_holder_at(i).get_control()
		assert_that(int(c.position.x)).is_equal(expected_x)
		assert_that(int(c.size.x)).is_equal(80)
		assert_that(int(c.size.y)).is_equal(360)
		expected_x += 80
	assert_that(rv.get_child_holder_count()).is_equal(8)

	# Scrolling moves the horizontal offset and the window slides.
	rv.scroll_horizontally(160)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(160)
	# Content = 100 * 80 = 8000; max horizontal offset = 8000 - 640 = 7360.
	rv.scroll_horizontally(10000)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(7360)
	rv.free_items()
	rv.free()


func test_horizontal_wheel_behavior_toggle() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(640, 360))
	var adapter := ItemAdapter.new()
	adapter.count = 100
	rv.set_item_size(80)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame

	# Default: the vertical wheel drives horizontal scrolling in a horizontal layout.
	var wheel_down := InputEventMouseButton.new()
	wheel_down.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel_down.pressed = true
	wheel_down.position = Vector2(100, 100)
	get_tree().root.push_input(wheel_down)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(48)

	# WHEEL_RIGHT (touchpad swipe / Shift+wheel) scrolls horizontally too.
	var wheel_right := InputEventMouseButton.new()
	wheel_right.button_index = MOUSE_BUTTON_WHEEL_RIGHT
	wheel_right.pressed = true
	wheel_right.position = Vector2(100, 100)
	get_tree().root.push_input(wheel_right)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(96)

	# Opting out: the vertical wheel is ignored, the horizontal wheel still works.
	rv.set_vertical_wheel_scrolls_horizontal(false)
	var wheel_down2 := InputEventMouseButton.new()
	wheel_down2.button_index = MOUSE_BUTTON_WHEEL_DOWN
	wheel_down2.pressed = true
	wheel_down2.position = Vector2(100, 100)
	get_tree().root.push_input(wheel_down2)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(96)

	var wheel_right2 := InputEventMouseButton.new()
	wheel_right2.button_index = MOUSE_BUTTON_WHEEL_RIGHT
	wheel_right2.pressed = true
	wheel_right2.position = Vector2(100, 100)
	get_tree().root.push_input(wheel_right2)
	await get_tree().process_frame
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(144)

	rv.free_items()
	rv.free()
