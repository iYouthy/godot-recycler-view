class_name ItemTouchAdapter
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
	(holder.get_control() as Label).text = "%d. %s" % [position, items[position]]


func move_item(from: int, to: int) -> void:
	var v: String = items[from]
	items.remove_at(from)
	items.insert(to, v)
	notify_item_moved(from, to)


func remove_item(pos: int) -> void:
	items.remove_at(pos)
	notify_item_removed(pos)
