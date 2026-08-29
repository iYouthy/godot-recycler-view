extends RecyclerView

const ITEM_SCENE := preload("res://test_list_item.tscn")

class MyAdapter extends Adapter:
	var items: Array = []

	func _get_item_count() -> int:
		return 50

	func _get_item_height(position: int) -> int:
		return 60

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		return MyViewHolder.new()


class MyViewHolder extends ViewHolder:
	func _init() -> void:
		control = ITEM_SCENE.instantiate()
		control.set_anchors_preset(Control.PRESET_FULL_RECT)


func _init() -> void:
	set_adapter(MyAdapter.new())
	set_layout(LinearLayoutManager.new())
