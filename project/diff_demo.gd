extends Control

const DiffAdapterScript := preload("res://diff_adapter.gd")

# DiffUtil demo: a small list with stable ids, deterministic edit buttons, and a
# log panel that prints the exact ops the diff dispatched. A "naive" button
# rebuilds the whole list instead, so the diff's minimal-op output is visible
# side by side with what a full relayout would cost.

# Describes the two lists for the diff. Items are compared by their stable id:
# same id + different text -> one change op (rebind), not a remove+insert.
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


# Dispatch sink: logs every op the diff emits and forwards it to the adapter as
# an incremental notify_* (exactly what AdapterListUpdateCallback does, plus a
# transcript for the panel).
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
		ops.append("修改 %d ×%d" % [position, count])
		adapter.notify_item_range_changed(position, count, payload)


@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel

var _adapter: DiffAdapterScript
var _log_callback: DiffLogCallback
var _next_id := 10


func _ready() -> void:
	_adapter = DiffAdapterScript.new()
	for i in 10:
		_adapter.items.append({"id": i, "text": "Item %d" % i})
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_item_animator(DefaultItemAnimator.new())

	_log_callback = DiffLogCallback.new()
	_log_callback.adapter = _adapter

	%InsertButton.pressed.connect(_on_insert_pressed)
	%RemoveButton.pressed.connect(_on_remove_pressed)
	%MoveButton.pressed.connect(_on_move_pressed)
	%ChangeButton.pressed.connect(_on_change_pressed)
	%MixButton.pressed.connect(_on_mix_pressed)
	%NaiveButton.pressed.connect(_on_naive_pressed)
	%ResetButton.pressed.connect(_on_reset_pressed)
	_update_info()


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	var lines: Array[String] = [
		"列表 %d 项 | 可见 %d | created %d（created 增长 = 真的重建了几项）" % [
			_adapter.items.size(), recycler_view.get_child_holder_count(), _adapter.created,
		],
	]
	if not _last_result.is_empty():
		lines.append_array(_last_result)
	info_label.text = "\n".join(lines)


# The latest dispatch transcript, shown under the summary line.
var _last_result: Array[String] = []


func _show_last_result(title: String, ops: Array, naive: bool) -> void:
	var lines: Array[String] = ["── %s ──" % title]
	for op in ops:
		lines.append("  " + str(op))
	if naive:
		lines.append("naive：全量重建 %d 项（所有可见项重新 _create_item）" % _adapter.items.size())
	elif ops.is_empty():
		lines.append("  无变化（diff 没有输出任何 op）")
	else:
		lines.append("共 %d op（naive 全量重建会重排 %d 项）" % [ops.size(), _adapter.items.size()])
	_last_result = lines


func _apply_diff(old: Array, new_list: Array, title: String) -> void:
	var callback := DiffCallback.new()
	callback.old_items = old
	callback.new_items = new_list
	# The adapter must report the new count while the diff is dispatched.
	_adapter.items = new_list
	_log_callback.ops.clear()
	var diff := DiffUtil.calculate_diff(callback, true)
	diff.dispatch_updates_to(_log_callback)
	_show_last_result(title, _log_callback.ops, false)


func _apply_naive(new_list: Array, title: String) -> void:
	_adapter.items = new_list
	recycler_view.notify_data_changed()
	_show_last_result(title + "（naive）", [], true)


func _on_insert_pressed() -> void:
	var old := _adapter.items.duplicate(true)
	var new_list := _adapter.items.duplicate(true)
	new_list.insert(3, {"id": _next_id, "text": "New %d" % _next_id})
	_next_id += 1
	_apply_diff(old, new_list, "在 index 3 插入")


func _on_remove_pressed() -> void:
	if _adapter.items.size() <= 6:
		return
	var old := _adapter.items.duplicate(true)
	var new_list := _adapter.items.duplicate(true)
	new_list.remove_at(5)
	_apply_diff(old, new_list, "删除 index 5")


func _on_move_pressed() -> void:
	if _adapter.items.size() < 9:
		return
	var old := _adapter.items.duplicate(true)
	var new_list := _adapter.items.duplicate(true)
	var value = new_list[2]
	new_list.remove_at(2)
	new_list.insert(8, value)
	_apply_diff(old, new_list, "移动 index 2 → 8")


func _on_change_pressed() -> void:
	if _adapter.items.is_empty():
		return
	var old := _adapter.items.duplicate(true)
	var new_list := _adapter.items.duplicate(true)
	new_list[4]["text"] += " ★"  # id unchanged: a change op, not a rebuild
	_apply_diff(old, new_list, "修改 index 4（id 不变）")


# Several edits at once so the diff merges them into the minimal op set.
func _mutate_mix(items: Array) -> void:
	items[4]["text"] += " ★"
	if items.size() > 3:
		items.remove_at(2)
	items.insert(0, {"id": _next_id, "text": "New %d" % _next_id})
	_next_id += 1


func _on_mix_pressed() -> void:
	var old := _adapter.items.duplicate(true)
	var new_list := _adapter.items.duplicate(true)
	_mutate_mix(new_list)
	_apply_diff(old, new_list, "混合：改 index4 + 删 index2 + 头部插入")


func _on_naive_pressed() -> void:
	var new_list := _adapter.items.duplicate(true)
	_mutate_mix(new_list)
	_apply_naive(new_list, "同样的混合编辑")


func _on_reset_pressed() -> void:
	_adapter.items = []
	for i in 10:
		_adapter.items.append({"id": i, "text": "Item %d" % i})
	recycler_view.notify_data_changed()
	_last_result = ["已重置为 10 项"]
