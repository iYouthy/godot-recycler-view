class_name CustomLayoutAdapter
extends Adapter

# Plain adapter for the custom-layout demo: 200 numbered rows. Nothing here is
# layout-specific — any adapter works with a custom LayoutManager.

var count := 200
var created := 0


func _get_item_count() -> int:
	return count


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var label := Label.new()
	label.set_size(Vector2(200, 40))
	label.add_theme_font_size_override("font_size", 18)
	vh.set_control(label)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	(holder.get_control() as Label).text = "第 %d 行" % position
