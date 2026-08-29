extends Control

const MixedAdapter := preload("res://mixed_adapter.gd")
const DividerDecoration := preload("res://divider_decoration.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: MixedAdapter


func _ready() -> void:
	_adapter = MixedAdapter.new()
	for i in 500:
		if i % 2 == 0:
			_adapter.items.append({
				"type": "user",
				"name": "User %d" % (i / 2),
				"age": 20 + (i % 50),
			})
		else:
			_adapter.items.append({
				"type": "message",
				"content": "Message %d: mixed-height list demo" % i,
				"is_read": i % 3 == 0,
			})
	var layout := LinearLayoutManager.new()
	recycler_view.set_item_extent(48)  # fallback height for items without one
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(layout)
	recycler_view.add_item_decoration(DividerDecoration.new())
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	var parts: Array = []
	for view_type in _adapter.created.keys():
		parts.append("type%d:%d" % [view_type, _adapter.created[view_type]])
	counter_label.text = "visible %d / %d | created %s" % [
		recycler_view.get_child_holder_count(), _adapter.items.size(), ", ".join(parts),
	]
