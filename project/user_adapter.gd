class_name UserAdapter
extends Adapter

# Each item is a User row with two child controls: an avatar Label and a name
# Label. A change op carrying a payload triggers a *partial* rebind via
# _bind_item_with_payload, which updates only the affected child control; the
# other child keeps its state. This is the RecyclerView/ListAdapter payload
# partial-update pattern.

var users: Array[Dictionary] = []  # {id: int, name: String, avatar: int}
var created := 0

const AVATARS := ["▲", "●", "■", "★"]
const AVATAR_COLORS := [Color(0.9, 0.3, 0.3), Color(0.3, 0.6, 0.9), Color(0.4, 0.8, 0.4), Color(0.9, 0.8, 0.2)]


func _get_item_count() -> int:
	return users.size()


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var row := HBoxContainer.new()
	row.set_size(Vector2(360, 40))
	var avatar := Label.new()
	avatar.name = "Avatar"
	avatar.set_size(Vector2(40, 40))
	var name_label := Label.new()
	name_label.name = "Name"
	name_label.set_size(Vector2(320, 40))
	row.add_child(avatar)
	row.add_child(name_label)
	vh.set_control(row)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	# Full rebind: set both child controls.
	var row: HBoxContainer = holder.get_control()
	var user: Dictionary = users[position]
	_set_avatar(row, user)
	(row.get_node("Name") as Label).text = user["name"]


func _bind_item_with_payload(holder: ViewHolder, position: int, payload: Variant) -> void:
	# Partial rebind: update only the child control(s) named by the payload.
	# The other child control is left untouched.
	var row: HBoxContainer = holder.get_control()
	var user: Dictionary = users[position]
	if payload is Array:
		for change in payload:
			match change:
				"avatar":
					_set_avatar(row, user)
				"name":
					(row.get_node("Name") as Label).text = user["name"]
		return
	_bind_item(holder, position)


func _set_avatar(row: HBoxContainer, user: Dictionary) -> void:
	var avatar := row.get_node("Avatar") as Label
	avatar.text = AVATARS[user["avatar"]]
	avatar.modulate = AVATAR_COLORS[user["avatar"]]
