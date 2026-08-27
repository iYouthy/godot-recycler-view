# Tests for payload partial updates: a User item has two child controls (avatar
# + name). A change op that carries a payload rebinds only the affected child
# control (_bind_item_with_payload), leaving the other child untouched.

extends GdUnitTestSuite

const UserAdapterScript := preload("res://user_adapter.gd")


class DiffCallback extends DiffUtilCallback:
	var old_users: Array[Dictionary] = []
	var new_users: Array[Dictionary] = []

	func _get_old_list_size() -> int:
		return old_users.size()

	func _get_new_list_size() -> int:
		return new_users.size()

	func _are_items_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_users[old_item_position]["id"] == new_users[new_item_position]["id"]

	func _are_contents_the_same(old_item_position: int, new_item_position: int) -> bool:
		var o := old_users[old_item_position]
		var n := new_users[new_item_position]
		return o["name"] == n["name"] and o["avatar"] == n["avatar"]

	func _get_change_payload(old_item_position: int, new_item_position: int) -> Variant:
		var o := old_users[old_item_position]
		var n := new_users[new_item_position]
		var changes: Array[String] = []
		if o["name"] != n["name"]:
			changes.append("name")
		if o["avatar"] != n["avatar"]:
			changes.append("avatar")
		return changes


class RecordingCallback extends ListUpdateCallback:
	var adapter: Adapter
	var payloads: Array = []

	func _on_inserted(position: int, count: int) -> void:
		adapter.notify_item_range_inserted(position, count)

	func _on_removed(position: int, count: int) -> void:
		adapter.notify_item_range_removed(position, count)

	func _on_moved(from_position: int, to_position: int) -> void:
		adapter.notify_item_moved(from_position, to_position)

	func _on_changed(position: int, count: int, payload: Variant) -> void:
		payloads.append(payload)
		adapter.notify_item_range_changed(position, count, payload)


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 600))
	var adapter := UserAdapterScript.new()
	var names := ["Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Heidi"]
	for i in names.size():
		adapter.users.append({"id": i, "name": names[i], "avatar": i % 4})
	rv.set_item_size(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.set_item_animator(DefaultItemAnimator.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func _dispatch(adapter: UserAdapterScript, new_list: Array) -> RecordingCallback:
	var cb := DiffCallback.new()
	cb.old_users = adapter.users
	cb.new_users = new_list
	adapter.users = new_list
	var rec := RecordingCallback.new()
	rec.adapter = adapter
	var diff := DiffUtil.calculate_diff(cb, true)
	diff.dispatch_updates_to(rec)
	return rec


func _row(rv: RecyclerView, pos: int) -> HBoxContainer:
	for i in rv.get_child_holder_count():
		var h = rv.get_child_holder_at(i)
		if h.get_position() == pos:
			return h.get_control() as HBoxContainer
	return null


func _avatar_text(rv: RecyclerView, pos: int) -> String:
	return (_row(rv, pos).get_node("Avatar") as Label).text


func _name_text(rv: RecyclerView, pos: int) -> String:
	return (_row(rv, pos).get_node("Name") as Label).text


func test_payload_name_updates_only_name_child() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: UserAdapterScript = s.adapter
	var created_before := adapter.created
	var avatar_before := _avatar_text(rv, 3)

	var new_list := adapter.users.duplicate(true)
	new_list[3]["name"] = "Dave Jr."
	var rec := _dispatch(adapter, new_list)
	assert_that(rec.payloads.size()).is_equal(1)
	assert_that(rec.payloads[0]).is_equal(["name"])

	await get_tree().process_frame  # notify defers the layout to end of frame
	# Only the name child changed; the avatar child is untouched.
	assert_that(_name_text(rv, 3)).is_equal("Dave Jr.")
	assert_that(_avatar_text(rv, 3)).is_equal(avatar_before)
	# No new holder was created for a partial update.
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_payload_avatar_updates_only_avatar_child() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: UserAdapterScript = s.adapter
	var created_before := adapter.created
	var name_before := _name_text(rv, 3)
	var avatar_before := _avatar_text(rv, 3)

	var new_list := adapter.users.duplicate(true)
	new_list[3]["avatar"] = (new_list[3]["avatar"] + 1) % 4
	var rec := _dispatch(adapter, new_list)
	assert_that(rec.payloads.size()).is_equal(1)
	assert_that(rec.payloads[0]).is_equal(["avatar"])

	await get_tree().process_frame
	assert_that(_avatar_text(rv, 3)).is_not_equal(avatar_before)
	assert_that(_name_text(rv, 3)).is_equal(name_before)
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_payload_array_updates_both_children() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: UserAdapterScript = s.adapter

	var avatar_before := _avatar_text(rv, 3)
	var new_list := adapter.users.duplicate(true)
	new_list[3]["name"] = "Dave *"
	new_list[3]["avatar"] = (new_list[3]["avatar"] + 2) % 4
	_dispatch(adapter, new_list)

	await get_tree().process_frame
	assert_that(_name_text(rv, 3)).is_equal("Dave *")
	assert_that(_avatar_text(rv, 3)).is_not_equal(avatar_before)
	rv.free_items()
	rv.free()


func test_null_payload_rebinds_whole_item() -> void:
	# A change notified with a null payload (no payload) falls back to a full
	# rebind instead of a partial one.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: UserAdapterScript = s.adapter

	var new_list := adapter.users.duplicate(true)
	new_list[3]["name"] = "Renamed"
	adapter.users = new_list
	adapter.notify_item_range_changed(3, 1, null)

	await get_tree().process_frame  # notify defers the layout to end of frame
	assert_that(_name_text(rv, 3)).is_equal("Renamed")
	rv.free_items()
	rv.free()
