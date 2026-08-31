extends RecyclerView

const ITEM_SCENE := preload("res://test_list_item.tscn")

class MyAdapter extends ListAdapter:
	func _create_item(_parent: Control, _view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		vh.set_control(ITEM_SCENE.instantiate())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var ctrl: TestListItem = holder.get_control()
		# await ctrl.ready
		ctrl.refresh(get_item(position))

func _ready() -> void:
	var adapter = MyAdapter.new()
	set_adapter(adapter)
	set_layout(LinearLayoutManager.new())
	set_scroll_bar(DefaultScrollBar.new())

	while adapter.get_current_list().size() < 10:
		await get_tree().create_timer(1).timeout
		var list := adapter.get_current_list().duplicate()
		list.append(str(Time.get_ticks_msec()))
		adapter.submit_list(list)
