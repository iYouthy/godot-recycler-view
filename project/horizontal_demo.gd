extends Control


@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: DemoAdapter

func _ready() -> void:
	_adapter = DemoAdapter.new()
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
