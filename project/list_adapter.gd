class_name ListAdapter
extends Adapter

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
