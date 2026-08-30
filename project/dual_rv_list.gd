extends HBoxContainer

@onready var left_rv: RecyclerView = $LeftRV
@onready var right_rv: RecyclerView = $RightRV

class LeftAdapter extends Adapter:
	func _get_item_count() -> int: return 10000
	func _get_item_extent(position: int) -> int: return 80
	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		vh.set_control(label)
		return vh
	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = "Item %d" % position
	
class RightAdapter extends Adapter:
	func _get_item_count() -> int: return 10000
	func _get_item_extent(position: int) -> int: return 80
	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		vh.set_control(label)
		return vh
	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = "Item %d" % position
	
func _ready() -> void:
	left_rv.set_adapter(LeftAdapter.new())
	left_rv.set_layout(LinearLayoutManager.new())
	left_rv.set_scroll_bar(DefaultScrollBar.new())
	
	right_rv.set_adapter(RightAdapter.new())
	right_rv.set_layout(LinearLayoutManager.new())
	right_rv.set_scroll_bar(DefaultScrollBar.new())
		
