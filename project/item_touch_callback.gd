class_name ItemTouchCallback
extends ItemTouchHelperCallback

var adapter: ItemTouchAdapter


func _get_movement_flags(holder: ViewHolder) -> int:
	# 长按可上下拖动换位，左滑/右滑删除。
	return ItemTouchHelper.make_movement_flags(
		ItemTouchHelper.UP | ItemTouchHelper.DOWN,
		ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT)


func _on_move(recycler_view, dragged: ViewHolder, target: ViewHolder) -> bool:
	adapter.move_item(dragged.get_position(), target.get_position())
	return true


func _on_swiped(holder: ViewHolder, direction: int) -> void:
	adapter.remove_item(holder.get_position())


func _on_selected_changed(holder: ViewHolder, action_state: int) -> void:
	if action_state == ItemTouchHelper.ACTION_STATE_DRAG:
		(holder.get_control() as Label).modulate = Color(0.8, 0.9, 1.0)


func _clear_view(holder: ViewHolder) -> void:
	(holder.get_control() as Label).modulate = Color.WHITE
