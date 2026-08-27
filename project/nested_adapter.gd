class_name NestedAdapter
extends Adapter

# A vertical feed whose first item holds a horizontal chip-row RV (perpendicular
# nesting) and whose second item holds a vertical sub-list RV (same-direction
# nesting with relay).

var count: int = 20
var created: Dictionary = {}


func _get_item_count() -> int:
	return count


func _get_item_view_type(position: int) -> int:
	if position == 0:
		return 1  # chip row (horizontal child)
	if position == 1:
		return 2  # nested vertical sub-list
	return 0  # plain label


func _get_item_height(position: int) -> int:
	if position == 0:
		return 64
	if position == 1:
		return 320
	return 48


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created[view_type] = created.get(view_type, 0) + 1
	var vh := ViewHolder.new()
	if view_type == 1:
		var root := Control.new()
		root.clip_contents = true  # keep the chip row inside the item's inset rect
		root.set_size(Vector2(360, 64))
		var chips := RecyclerView.new()
		chips.position = Vector2(0, 0)
		chips.set_size(Vector2(360, 64))
		chips.set_item_size(48)
		chips.set_adapter(ChipAdapter.new())
		var layout := LinearLayoutManager.new()
		layout.set_orientation(LinearLayoutManager.HORIZONTAL)
		chips.set_layout(layout)
		chips.request_layout()
		root.add_child(chips)
		vh.set_control(root)
	elif view_type == 2:
		var root := Control.new()
		root.clip_contents = true  # keep the sub-list inside the item's inset rect
		root.set_size(Vector2(360, 120))
		var sub := RecyclerView.new()
		sub.position = Vector2(0, 0)
		sub.set_size(Vector2(360, 320))
		sub.set_item_size(36)
		sub.set_adapter(SubAdapter.new())
		sub.set_layout(LinearLayoutManager.new())
		sub.request_layout()
		root.add_child(sub)
		vh.set_control(root)
	else:
		var label := Label.new()
		label.set_size(Vector2(360, 48))
		vh.set_control(label)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var control: Control = holder.get_control()
	if control is Label:
		control.text = "Item %d" % position


class ChipAdapter extends Adapter:
	var count: int = 12

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(48, 64))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = "C%d" % position


class SubAdapter extends Adapter:
	var count: int = 15

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(360, 36))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var label: Label = holder.get_control()
		label.text = "Sub %d" % position
