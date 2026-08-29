extends Control

const ItemTouchAdapter := preload("res://item_touch_adapter.gd")
const ItemTouchCallback := preload("res://item_touch_callback.gd")

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var counter_label: Label = %CounterLabel

var _adapter: ItemTouchAdapter


func _ready() -> void:
	_adapter = ItemTouchAdapter.new()
	for i in 12:
		_adapter.items.append("Item %d" % i)
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	# 拖拽让位 / 删除淡出与 ItemAnimator 天然配合。
	recycler_view.set_item_animator(DefaultItemAnimator.new())

	var callback := ItemTouchCallback.new()
	callback.adapter = _adapter
	var helper := ItemTouchHelper.new()
	helper.set_callback(callback)
	helper.attach_to_recycler_view(recycler_view)
	_update_counter()


func _process(_delta: float) -> void:
	_update_counter()


func _update_counter() -> void:
	counter_label.text = "%d items | visible %d | created %d — 长按拖动换位，左滑/右滑删除" % [
		_adapter.items.size(), recycler_view.get_child_holder_count(), _adapter.created,
	]
