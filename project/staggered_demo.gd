extends Control

const StaggeredAdapter := preload("res://staggered_adapter.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: StaggeredAdapter


func _ready() -> void:
	_adapter = StaggeredAdapter.new()
	for i in 200:
		_adapter.items.append("item %d" % i)
	recycler_view.set_adapter(_adapter)
	var layout := StaggeredGridLayoutManager.new()
	layout.set_span_count(3)
	recycler_view.set_layout(layout)
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	counter_label.text = "%d items | visible %d | created %d — 3 列瀑布流，item 高度不一" % [
		_adapter.items.size(), recycler_view.get_child_holder_count(), _adapter.created,
	]
