# Tests for the DiffUtil application path used by diff_demo: a list whose items
# are compared by stable id, so changes reuse their view holder instead of
# rebuilding it, and moves dispatch as a single move op instead of a rebuild.

extends GdUnitTestSuite

const DiffAdapterScript := preload("res://diff_adapter.gd")


class DiffCallback extends DiffUtilCallback:
	var old_items: Array[Dictionary] = []
	var new_items: Array[Dictionary] = []

	func _get_old_list_size() -> int:
		return old_items.size()

	func _get_new_list_size() -> int:
		return new_items.size()

	func _are_items_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_items[old_item_position]["id"] == new_items[new_item_position]["id"]

	func _are_contents_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_items[old_item_position]["text"] == new_items[new_item_position]["text"]


# Records the kinds of ops the diff dispatched (one string per op) AND forwards
# each op to the adapter, so the RecyclerView actually processes the diff (the
# same pattern diff_demo uses: dispatch sink + adapter forwarding).
class RecordingCallback extends ListUpdateCallback:
	var adapter: Adapter
	var ops: Array[String] = []

	func _on_inserted(position: int, count: int) -> void:
		ops.append("insert")
		adapter.notify_item_range_inserted(position, count)

	func _on_removed(position: int, count: int) -> void:
		ops.append("remove")
		adapter.notify_item_range_removed(position, count)

	func _on_moved(from_position: int, to_position: int) -> void:
		ops.append("move")
		adapter.notify_item_moved(from_position, to_position)

	func _on_changed(position: int, count: int, payload: Variant) -> void:
		ops.append("change")
		adapter.notify_item_range_changed(position, count, payload)


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := DiffAdapterScript.new()
	for i in 10:
		adapter.items.append({"id": i, "text": "Item %d" % i})
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func _dispatch(adapter: DiffAdapterScript, new_list: Array) -> RecordingCallback:
	var cb := DiffCallback.new()
	cb.old_items = adapter.items
	cb.new_items = new_list
	adapter.items = new_list
	var rec := RecordingCallback.new()
	rec.adapter = adapter
	var diff := DiffUtil.calculate_diff(cb, true)
	diff.dispatch_updates_to(rec)
	return rec


func _any_text_has(rv: RecyclerView, needle: String) -> bool:
	for i in rv.get_child_holder_count():
		var c: Control = rv.get_child_holder_at(i).get_control()
		if c is Label and (c as Label).text.contains(needle):
			return true
	return false


func test_change_by_stable_id_rebinds_not_recreates() -> void:
	# Editing an item's text keeps its id: the diff must dispatch a change op and
	# rebind the existing holder, not remove+insert a fresh one.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: DiffAdapterScript = s.adapter
	var created_before := adapter.created

	var new_list := adapter.items.duplicate(true)
	new_list[4]["text"] += " ★"
	var rec := _dispatch(adapter, new_list)

	assert_that(rec.ops).contains("change")
	await get_tree().process_frame
	# The rebind updated the visible text...
	assert_that(_any_text_has(rv, "★")).is_true()
	# ...without creating a single new holder.
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_move_emits_single_move_not_rebuild() -> void:
	# Moving one item must dispatch one move op, not a remove+insert pair that
	# would recreate the moved view.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: DiffAdapterScript = s.adapter
	var created_before := adapter.created

	var new_list := adapter.items.duplicate(true)
	var value = new_list[2]
	new_list.remove_at(2)
	new_list.insert(8, value)
	var rec := _dispatch(adapter, new_list)

	assert_that(rec.ops).contains("move")
	assert_that(rec.ops.filter(func(o: String): return o == "remove").size()).is_equal(0)
	assert_that(rec.ops.filter(func(o: String): return o == "insert").size()).is_equal(0)
	await get_tree().process_frame
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_mixed_edits_dispatch_far_less_than_naive() -> void:
	# Several edits at once still merge into a handful of ops, not a full
	# rebuild of every visible item (the naive notify_data_changed path).
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: DiffAdapterScript = s.adapter
	var created_before := adapter.created

	var new_list := adapter.items.duplicate(true)
	new_list[4]["text"] += " ★"
	new_list.remove_at(2)
	new_list.insert(0, {"id": 100, "text": "New 100"})
	var rec := _dispatch(adapter, new_list)

	assert_that(rec.ops.size()).is_less(adapter.items.size())
	await get_tree().process_frame
	assert_that(adapter.created).is_equal(created_before + 1)  # only the insert is new
	rv.free_items()
	rv.free()
