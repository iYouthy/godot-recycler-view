extends Control

const UserAdapterScript := preload("res://user_adapter.gd")

# Partial-update demo: each User item has two child controls (avatar + name).
# When only one field changes, the diff keeps the stable id (_are_items_the_same
# is true) and _get_change_payload describes which field changed. The change op
# therefore triggers a *partial* rebind that updates only that child control,
# leaving the other child untouched.

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

	# Describes which child control(s) changed, so the rebind can update only those.
	func _get_change_payload(old_item_position: int, new_item_position: int) -> Variant:
		var o := old_users[old_item_position]
		var n := new_users[new_item_position]
		var changes: Array[String] = []
		if o["name"] != n["name"]:
			changes.append("name")
		if o["avatar"] != n["avatar"]:
			changes.append("avatar")
		return changes


# Dispatch sink: logs every op the diff emits and forwards it to the adapter.
class DiffLogCallback extends ListUpdateCallback:
	var adapter: Adapter
	var ops: Array[String] = []

	func _on_inserted(position: int, count: int) -> void:
		ops.append("插入 %d ×%d" % [position, count])
		adapter.notify_item_range_inserted(position, count)

	func _on_removed(position: int, count: int) -> void:
		ops.append("删除 %d ×%d" % [position, count])
		adapter.notify_item_range_removed(position, count)

	func _on_moved(from_position: int, to_position: int) -> void:
		ops.append("移动 %d → %d" % [from_position, to_position])
		adapter.notify_item_moved(from_position, to_position)

	func _on_changed(position: int, count: int, payload: Variant) -> void:
		ops.append("修改 %d ×%d (payload=%s)" % [position, count, str(payload)])
		adapter.notify_item_range_changed(position, count, payload)


@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel

var _adapter: UserAdapterScript
var _log_callback: DiffLogCallback


func _ready() -> void:
	_adapter = UserAdapterScript.new()
	var names := ["Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Heidi"]
	for i in names.size():
		_adapter.users.append({"id": i, "name": names[i], "avatar": i % 4})
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_item_animator(DefaultItemAnimator.new())

	_log_callback = DiffLogCallback.new()
	_log_callback.adapter = _adapter

	%ChangeNameButton.pressed.connect(_on_change_name_pressed)
	%ChangeAvatarButton.pressed.connect(_on_change_avatar_pressed)
	%ChangeBothButton.pressed.connect(_on_change_both_pressed)
	%ResetButton.pressed.connect(_on_reset_pressed)
	_update_info()


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	var lines: Array[String] = [
		"列表 %d 项 | created %d（created 增长 = 真的重建了几项）" % [_adapter.users.size(), _adapter.created],
	]
	if not _last_result.is_empty():
		lines.append_array(_last_result)
	info_label.text = "\n".join(lines)


var _last_result: Array[String] = []


func _show_last_result(title: String, ops: Array) -> void:
	var lines: Array[String] = ["── %s ──" % title]
	for op in ops:
		lines.append("  " + str(op))
	if ops.is_empty():
		lines.append("  无变化")
	lines.append("  局部更新：只改 payload 列出的子控件，另一个子控件不动，created 不变")
	_last_result = lines


func _apply_diff(old: Array, new_list: Array, title: String) -> void:
	var callback := DiffCallback.new()
	callback.old_users = old
	callback.new_users = new_list
	_adapter.users = new_list
	_log_callback.ops.clear()
	var diff := DiffUtil.calculate_diff(callback, true)
	diff.dispatch_updates_to(_log_callback)
	_show_last_result(title, _log_callback.ops)


func _on_change_name_pressed() -> void:
	if _adapter.users.size() < 4:
		return
	var old := _adapter.users.duplicate(true)
	var new_list := _adapter.users.duplicate(true)
	new_list[3]["name"] = new_list[3]["name"] + " Jr."
	_apply_diff(old, new_list, "改第3项名字（id 不变）")


func _on_change_avatar_pressed() -> void:
	if _adapter.users.size() < 4:
		return
	var old := _adapter.users.duplicate(true)
	var new_list := _adapter.users.duplicate(true)
	new_list[3]["avatar"] = (new_list[3]["avatar"] + 1) % 4
	_apply_diff(old, new_list, "改第3项头像（id 不变）")


func _on_change_both_pressed() -> void:
	if _adapter.users.size() < 4:
		return
	var old := _adapter.users.duplicate(true)
	var new_list := _adapter.users.duplicate(true)
	new_list[3]["name"] = new_list[3]["name"] + " *"
	new_list[3]["avatar"] = (new_list[3]["avatar"] + 2) % 4
	_apply_diff(old, new_list, "改第3项名字+头像（payload 数组含两项）")


func _on_reset_pressed() -> void:
	_adapter.users = []
	var names := ["Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Heidi"]
	for i in names.size():
		_adapter.users.append({"id": i, "name": names[i], "avatar": i % 4})
	recycler_view.notify_data_changed()
	_last_result = ["已重置"]
