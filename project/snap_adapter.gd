extends Adapter

const CARD_SCENE := preload("res://snap_demo_card.tscn")

var items: Array = []
var created := 0


func _get_item_count() -> int:
	return items.size()


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var rect := CARD_SCENE.instantiate()
	vh.set_control(rect)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var card := holder.get_control()
	(card.find_child("Item") as Label).text = items[position]
