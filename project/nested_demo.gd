extends Control

const NestedAdapter := preload("res://nested_adapter.gd")
const DividerDecoration := preload("res://divider_decoration.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: NestedAdapter


func _ready() -> void:
	_adapter = NestedAdapter.new()
	recycler_view.set_item_size(48)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.add_item_decoration(DividerDecoration.new())
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	var parts: Array = []
	for view_type in _adapter.created.keys():
		parts.append("type%d:%d" % [view_type, _adapter.created[view_type]])
	counter_label.text = "visible %d / %d | %s | state %d" % [
		recycler_view.get_child_holder_count(), _adapter.count, ", ".join(parts),
		recycler_view.get_scroll_state(),
	]
