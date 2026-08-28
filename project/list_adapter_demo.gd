extends Control

# ListAdapter demo: submit_list() diffs the current list against the new one
# internally and dispatches incremental updates automatically - no manual
# DiffCallback + dispatch wiring, no _get_item_count override (the ListAdapter
# provides it from the current list).

class UserCallback extends DiffUtilItemCallback:
	func _are_items_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["id"] == new_item["id"]

	func _are_contents_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["name"] == new_item["name"]


class UserListAdapter extends ListAdapter:
	var created := 0

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(360, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var user: Dictionary = get_item(position)
		(holder.get_control() as Label).text = "%s (id %d)" % [user["name"], user["id"]]


@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel

var _adapter: UserListAdapter
var _last_action := ""


func _ready() -> void:
	_adapter = UserListAdapter.new()
	_adapter.set_diff_callback(UserCallback.new())
	recycler_view.set_item_size(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_item_animator(DefaultItemAnimator.new())

	%RandomButton.pressed.connect(_on_random_pressed)
	%AppendButton.pressed.connect(_on_append_pressed)
	%RenameButton.pressed.connect(_on_rename_pressed)
	%RemoveButton.pressed.connect(_on_remove_pressed)
	_update_info()


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	info_label.text = "列表 %d 项 | created %d（created 增长 = 重建）\n上次：%s" % [
		_adapter.get_item_count(), _adapter.created, _last_action,
	]


const _names := ["Alice", "Bob", "Carol", "Dave", "Eve", "Frank", "Grace", "Heidi"]


func _random_users(n: int) -> Array:
	var arr := []
	for i in n:
		arr.append({"id": randi() % 1000, "name": _names[i % _names.size()]})
	return arr


func _on_random_pressed() -> void:
	_last_action = "submit 随机列表（新 id，旧项会重建）"
	_adapter.submit_list(_random_users(3 + randi() % 6))


func _on_append_pressed() -> void:
	var arr := _adapter.get_current_list().duplicate(true)
	arr.append({"id": randi() % 1000, "name": "New%d" % arr.size()})
	_last_action = "追加一项（其余不动）"
	_adapter.submit_list(arr)


func _on_rename_pressed() -> void:
	var arr := _adapter.get_current_list().duplicate(true)
	if arr.is_empty():
		return
	arr[0]["name"] = arr[0]["name"] + " *"
	_last_action = "改第一项名字（id 不变 → change）"
	_adapter.submit_list(arr)


func _on_remove_pressed() -> void:
	var arr := _adapter.get_current_list().duplicate(true)
	if arr.is_empty():
		return
	arr.pop_back()
	_last_action = "删最后一项"
	_adapter.submit_list(arr)
