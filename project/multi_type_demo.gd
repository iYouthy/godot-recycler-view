extends Control

const MultiTypeAdapter := preload("res://multi_type_adapter.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: MultiTypeAdapter


func _ready() -> void:
	_adapter = MultiTypeAdapter.new()
	_adapter.count = 1000
	var layout := LinearLayoutManager.new()
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(layout)
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	var parts: Array = []
	for view_type in _adapter.created.keys():
		parts.append("type%d:%d" % [view_type, _adapter.created[view_type]])
	counter_label.text = "visible %d / %d items | created %s" % [
		recycler_view.get_child_holder_count(), _adapter.count, ", ".join(parts),
	]
