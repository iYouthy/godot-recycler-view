class_name MixedAdapter
extends Adapter

# A mixed list of two view types, each backed by its own PackedScene with a
# different height: users are tall (avatar + name + age), messages are short.
const TYPE_USER := 0
const TYPE_MESSAGE := 1
const USER_HEIGHT := 72
const MESSAGE_HEIGHT := 48

const _user_scene := preload("res://user_item.tscn")
const _message_scene := preload("res://message_item.tscn")

var items: Array = []
# view_type -> number of fresh views created for it (bounded by reuse).
var created: Dictionary = {}


func _get_item_count() -> int:
	return items.size()


func _get_item_view_type(position: int) -> int:
	return TYPE_USER if items[position]["type"] == "user" else TYPE_MESSAGE


func _get_item_extent(position: int) -> int:
	return USER_HEIGHT if _get_item_view_type(position) == TYPE_USER else MESSAGE_HEIGHT


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created[view_type] = created.get(view_type, 0) + 1
	var vh := ViewHolder.new()
	var scene: PackedScene = _user_scene if view_type == TYPE_USER else _message_scene
	vh.set_control(scene.instantiate() as Control)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var data: Dictionary = items[position]
	var item: Control = holder.get_control()
	if data["type"] == "user":
		var avatar: ColorRect = item.get_node("Avatar")
		avatar.color = Color.from_hsv(fmod(position * 0.13, 1.0), 0.5, 0.8)
		(item.get_node("Name") as Label).text = data["name"]
		(item.get_node("Age") as Label).text = "Age %d" % data["age"]
	else:
		(item.get_node("Content") as Label).text = data["content"]
		(item.get_node("Status") as Label).text = "read" if data["is_read"] else "unread"
