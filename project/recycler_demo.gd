extends Control

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: DemoAdapter
var _update_callback: AdapterListUpdateCallback
var _frame := 0


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


func _ready() -> void:
	_adapter = DemoAdapter.new()
	for i in 10000:
		_adapter.items.append(i)
	var layout := LinearLayoutManager.new()
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(layout)
	recycler_view.set_item_animator(DefaultItemAnimator.new())
	recycler_view.set_scroll_bar(DefaultScrollBar.new())
	_update_callback = AdapterListUpdateCallback.new()
	_update_callback.set_adapter(_adapter)
	counter_label.text = "created holders: %d" % _adapter.created


func _process(_delta: float) -> void:
	_frame += 1
	if _frame % 90 == 0:
		_mutate_and_dispatch()
	counter_label.text = "created holders: %d (only ~%d instantiated for %d items)" % [
		_adapter.created, recycler_view.get_child_holder_count(), _adapter.items.size(),
	]


func _mutate_and_dispatch() -> void:
	var old := _adapter.items.duplicate()
	var new_list: Array = _adapter.items.duplicate()
	_mutate(new_list)

	var callback := DiffCallback.new()
	callback.old_items = old
	callback.new_items = new_list
	# The adapter must report the new count while the diff is dispatched.
	_adapter.items = new_list
	var diff := DiffUtil.calculate_diff(callback, true)
	diff.dispatch_updates_to(_update_callback)


# Applies 1-3 random edits so the diff shows real insert/remove/move/change ops.
func _mutate(items: Array) -> void:
	var op_count := 1 + randi() % 3
	for i in op_count:
		if items.is_empty():
			return
		var op := randi() % 4
		match op:
			0:  # insert a fresh value
				var pos := randi() % (items.size() + 1)
				items.insert(pos, -items.size())
			1:  # remove a random item
				var pos := randi() % items.size()
				items.remove_at(pos)
			2:  # move one item elsewhere
				var from := randi() % items.size()
				var to := randi() % items.size()
				var value = items[from]
				items.remove_at(from)
				items.insert(to, value)
			_:  # change a value (negative so it stands out)
				var pos := randi() % items.size()
				items[pos] = -items[pos]


class DemoAdapter extends Adapter:
	var items: Array = []
	var created: int = 0

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_anchors_preset(Control.PRESET_FULL_RECT)
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = "Item %d" % items[position]
