class_name GridAdapter
extends Adapter

# Grid items: every 10th position is a full-width section header (spans the whole
# row), the rest are single-column cells with slightly varying heights so rows
# take the height of their tallest cell.
const HEADER_EVERY := 10

var count: int = 0
var created: int = 0


class SpanLookup extends SpanSizeLookup:
	func _get_span_size(position: int) -> int:
		return 3 if position % 10 == 0 else 1


func _get_item_count() -> int:
	return count


func _get_item_view_type(position: int) -> int:
	return 0 if position % HEADER_EVERY == 0 else 1


func _get_item_extent(position: int) -> int:
	return 80 if position % HEADER_EVERY == 0 else 50 + (position % 3) * 10


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	vh.set_control(load("res://grid_item.tscn").instantiate() as Control)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var item: Control = holder.get_control()
	var label: Label = item.get_node("Label")
	var color: ColorRect = item.get_node("Color")
	if position % HEADER_EVERY == 0:
		label.text = "Section %d" % (position / HEADER_EVERY)
		color.color = Color(0.2, 0.4, 0.7)
	else:
		label.text = "Item %d" % position
		color.color = Color.from_hsv(fmod(position * 0.07, 1.0), 0.4, 0.6)
