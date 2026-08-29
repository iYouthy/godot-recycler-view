extends Control

const OpsAdapter := preload("res://ops_adapter.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: OpsAdapter


func _ready() -> void:
	_adapter = OpsAdapter.new()
	for i in 8:
		_adapter.items.append("Item %d" % i)
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_item_animator(DefaultItemAnimator.new())

	%InsertButton.pressed.connect(_on_insert_pressed)
	%RemoveButton.pressed.connect(_on_remove_pressed)
	%MoveButton.pressed.connect(_on_move_pressed)
	%ChangeButton.pressed.connect(_on_change_pressed)
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	counter_label.text = "%d items | visible %d | created %d" % [
		_adapter.items.size(), recycler_view.get_child_holder_count(), _adapter.created,
	]


func _on_insert_pressed() -> void:
	_adapter.items.insert(0, "New %d" % _adapter.created)
	recycler_view.notify_item_range_inserted(0, 1)


func _on_remove_pressed() -> void:
	if _adapter.items.size() > 1:
		_adapter.items.remove_at(2)
		recycler_view.notify_item_range_removed(2, 1)


func _on_move_pressed() -> void:
	if _adapter.items.size() > 2:
		var value = _adapter.items[0]
		_adapter.items.remove_at(0)
		_adapter.items.append(value)
		recycler_view.notify_item_moved(0, _adapter.items.size() - 1)


func _on_change_pressed() -> void:
	if _adapter.items.size() > 1:
		_adapter.items[1] = "Updated %d" % _adapter.created
		recycler_view.notify_item_range_changed(1, 1, null)
