# Tests for ListAdapter: submit_list() diffs the current list against the new
# one and dispatches incremental updates automatically, using a
# DiffUtilItemCallback for item comparison.

extends GdUnitTestSuite


class ItemCallback extends DiffUtilItemCallback:
	func _are_items_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["id"] == new_item["id"]

	func _are_contents_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["name"] == new_item["name"]

	func _get_change_payload(old_item: Variant, new_item: Variant) -> Variant:
		if old_item["name"] != new_item["name"]:
			return "name"
		return null


class ListAdapterImpl extends ListAdapter:
	var created := 0
	var partial_calls := 0

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(get_item(position)["name"])

	func _bind_item_with_payload(holder: ViewHolder, position: int, payload: Variant) -> void:
		partial_calls += 1
		(holder.get_control() as Label).text = str(get_item(position)["name"])


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := ListAdapterImpl.new()
	adapter.set_diff_callback(ItemCallback.new())
	rv.set_item_size(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func _items(n: int) -> Array:
	var arr := []
	for i in n:
		arr.append({"id": i, "name": "a%d" % i})
	return arr


func _text(rv: RecyclerView, pos: int) -> String:
	for i in rv.get_child_holder_count():
		var h = rv.get_child_holder_at(i)
		if h.get_position() == pos:
			return (h.get_control() as Label).text
	return ""


func test_submit_initial_populates() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	assert_that(adapter.get_item_count()).is_equal(0)

	adapter.submit_list(_items(5))
	await get_tree().process_frame
	assert_that(adapter.get_item_count()).is_equal(5)
	assert_that(rv.get_child_holder_count()).is_equal(5)
	assert_that(_text(rv, 0)).is_equal("a0")
	assert_that(_text(rv, 4)).is_equal("a4")
	rv.free_items()
	rv.free()


func test_submit_change_updates_in_place() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	adapter.submit_list(_items(5))
	await get_tree().process_frame
	var created_before := adapter.created

	var new_list := _items(5)
	new_list[2]["name"] = "changed"
	adapter.submit_list(new_list)
	await get_tree().process_frame
	# The changed item's text updated without creating a new holder.
	assert_that(_text(rv, 2)).is_equal("changed")
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_submit_remove_then_insert() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	adapter.submit_list(_items(5))
	await get_tree().process_frame

	adapter.submit_list(_items(3))  # two items removed
	await get_tree().process_frame
	assert_that(rv.get_child_holder_count()).is_equal(3)
	assert_that(_text(rv, 2)).is_equal("a2")

	adapter.submit_list(_items(4))  # one item inserted
	await get_tree().process_frame
	assert_that(rv.get_child_holder_count()).is_equal(4)
	assert_that(_text(rv, 3)).is_equal("a3")
	rv.free_items()
	rv.free()


func test_submit_same_instance_is_noop() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	adapter.submit_list(_items(3))
	await get_tree().process_frame
	var created_before := adapter.created

	var same := adapter.get_current_list()  # the same Array instance
	adapter.submit_list(same)
	await get_tree().process_frame
	assert_that(adapter.created).is_equal(created_before)
	assert_that(rv.get_child_holder_count()).is_equal(3)
	rv.free_items()
	rv.free()


func test_get_item_and_current_list() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	adapter.submit_list(_items(4))
	await get_tree().process_frame

	assert_that(adapter.get_item(2)["name"]).is_equal("a2")
	assert_that(adapter.get_current_list().size()).is_equal(4)
	rv.free_items()
	rv.free()


func test_submit_payload_triggers_partial_bind() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: ListAdapterImpl = s.adapter
	adapter.submit_list(_items(3))
	await get_tree().process_frame
	var created_before := adapter.created

	# A change op with a payload reaches _bind_item_with_payload.
	var new_list := _items(3)
	new_list[1]["name"] = "renamed"
	adapter.submit_list(new_list)
	await get_tree().process_frame
	assert_that(adapter.partial_calls).is_greater(0)
	assert_that(_text(rv, 1)).is_equal("renamed")
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()
