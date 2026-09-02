extends Control

@onready var recycler_view: RecyclerView = %RecyclerView

# 自定义滚动条（16px 胶囊样式，见 custom_scroll_bar.gd）。
# RV 的滚动条是内置的 VScrollBar/HScrollBar（RV 构造时自动创建），
# 直接用 get_v_scroll_bar() 取到后主题化即可，无需子类。
const CustomScrollBarTheme := preload("res://custom_scroll_bar.gd")

class DemoAdapter extends Adapter:
	var count: int = 20
	var created := 0

	func _get_item_count() -> int:
		return count

	func _get_item_extent(_p: int) -> int:
		return 60

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var box := PanelContainer.new()
		var label := Label.new()
		label.size_flags_vertical = Control.SIZE_EXPAND_FILL
		label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		box.add_child(label)
		vh.set_control(box)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var box :PanelContainer = holder.control
		var label: Label = box.get_child(0)
		label.text = "Item %d" % position


func _ready() -> void:
	var adapter := DemoAdapter.new()
	recycler_view.set_item_extent(60)
	recycler_view.set_adapter(adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	CustomScrollBarTheme.style(recycler_view.get_v_scroll_bar())
	# 常显方便观察（Auto 模式默认只在内容溢出时显示）。
	recycler_view.set_scroll_bar_auto_hide(false)
