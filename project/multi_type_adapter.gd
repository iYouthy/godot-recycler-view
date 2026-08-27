class_name MultiTypeAdapter
extends Adapter

# Three view types (header / body / footer) in a repeating pattern, each with its
# own color and text prefix. All share the RecyclerView's fixed item height.
const COLORS := {
	0: Color(0.45, 0.75, 1.0),  # header - blue
	1: Color(1.0, 0.9, 0.6),    # body - amber
	2: Color(0.7, 1.0, 0.7),    # footer - green
}
const PREFIXES := { 0: "Header", 1: "Item", 2: "Footer" }

var count: int = 0
# view_type -> number of fresh views created for it (bounded by reuse).
var created: Dictionary = {}


func _get_item_count() -> int:
	return count


func _get_item_view_type(position: int) -> int:
	return position % 3


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created[view_type] = created.get(view_type, 0) + 1
	var vh := ViewHolder.new()
	var label := Label.new()
	label.set_anchors_preset(Control.PRESET_FULL_RECT)
	label.add_theme_color_override("font_color", COLORS.get(view_type, Color.WHITE))
	vh.set_control(label)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var view_type: int = holder.get_item_view_type()
	var label: Label = holder.get_control()
	label.text = "%s %d (type %d)" % [PREFIXES.get(view_type, "?"), position, view_type]
