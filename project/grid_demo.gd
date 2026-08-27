extends Control

const GridAdapter := preload("res://grid_adapter.gd")
const DividerDecoration := preload("res://divider_decoration.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: GridAdapter


func _ready() -> void:
	_adapter = GridAdapter.new()
	_adapter.count = 100
	var layout := GridLayoutManager.new()
	layout.set_span_count(3)
	layout.set_span_size_lookup(GridAdapter.SpanLookup.new())
	recycler_view.set_item_size(60)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(layout)
	recycler_view.add_item_decoration(DividerDecoration.new())
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	counter_label.text = "visible %d / %d | created %d" % [
		recycler_view.get_child_holder_count(), _adapter.count, _adapter.created,
	]
