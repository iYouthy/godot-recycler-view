extends Control

@onready var recycler_view: RecyclerView = %RecyclerView

# 自定义滚动条（16px 胶囊样式，见 custom_scroll_bar.gd）。
# 用法：recycler_view.set_scroll_bar(你的滚动条实例)。
# 继承 RecyclerViewScrollBar 完全自定义（自己画 + 自己处理事件），
# 或继承 DefaultScrollBar 只改样式（交互全保留）。
const CustomScrollBar := preload("res://custom_scroll_bar.gd")

class DemoAdapter extends Adapter:
	var count: int = 1000
	var created := 0

	func _get_item_count() -> int:
		return count

	func _get_item_height(_p: int) -> int:
		return 60

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 60))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = "Item %d" % position


func _ready() -> void:
	var adapter := DemoAdapter.new()
	recycler_view.set_item_size(60)
	recycler_view.set_adapter(adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_scroll_bar(CustomScrollBar.new())
