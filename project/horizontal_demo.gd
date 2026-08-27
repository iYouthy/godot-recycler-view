extends Control

const ListAdapter := preload("res://list_adapter.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: ListAdapter


func _ready() -> void:
	_adapter = ListAdapter.new()
	for i in 1000:
		_adapter.items.append(i)
	var layout := LinearLayoutManager.new()
	layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	recycler_view.set_item_size(80)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(layout)
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	counter_label.text = "visible %d / %d items | created %d | h-offset %d" % [
		recycler_view.get_child_holder_count(), _adapter.items.size(), _adapter.created,
		recycler_view.get_scroll_offset_horizontal(),
	]
