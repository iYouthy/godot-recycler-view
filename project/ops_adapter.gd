class_name OpsAdapter
extends Adapter

var items: Array[String] = []
var created := 0


func _get_item_count() -> int:
	return items.size()


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var label := Label.new()
	label.set_size(Vector2(360, 40))
	vh.set_control(label)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var label: Label = holder.get_control()
	label.text = items[position]
