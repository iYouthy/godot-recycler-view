class_name DividerDecoration
extends ItemDecoration

# Draws a thin horizontal divider under every item (list or grid cell).
const GAP := 6

func _get_item_offsets(position: int, parent: Control) -> Vector4:
	return Vector4(0, 0, 0, GAP)


func _on_draw(parent: Control) -> void:
	var rv: RecyclerView = parent
	for i in rv.get_child_holder_count():
		var holder: ViewHolder = rv.get_child_holder_at(i)
		var rect := rv.get_decorated_item_rect(holder.get_position())
		var y := rect.position.y + rect.size.y + GAP * 0.5
		parent.draw_line(
			Vector2(rect.position.x, y),
			Vector2(rect.position.x + rect.size.x, y),
			Color(0.6, 0.6, 0.6, 0.4),
			1,
		)
