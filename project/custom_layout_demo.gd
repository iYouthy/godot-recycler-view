extends Control

const CustomLayoutManagerScript := preload("res://custom_layout_manager.gd")
const CustomLayoutAdapterScript := preload("res://custom_layout_adapter.gd")

# Custom-layout demo: the list below is laid out by a GDScript subclass of
# LayoutManager (custom_layout_manager.gd) — rows drift sideways in a sine wave.
#
# What to look for:
#   - 打开 custom_layout_manager.gd：全部注释都在里面。核心只有
#     _on_layout_children() —— 读 offset → 回收越界项 → 补齐可见项 → 定位。
#   - 滚轮/拖拽滚动：偏移由 RecyclerView 统一管理，每次变化重跑布局；
#     滚动范围来自 _get_content_size()，scroll_to_position 走 _get_position_offset()。
#   - 数字"复用中"不变大：滚出视口的行被脚本回收进缓存/池，滚回来时复用。
#   - 换布局 = 换一个脚本：把波浪公式换成任何你想要的排布即可。

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel

var _layout: CustomLayoutManagerScript
var _adapter: CustomLayoutAdapterScript


func _ready() -> void:
	_layout = CustomLayoutManagerScript.new()
	_adapter = CustomLayoutAdapterScript.new()
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(_layout)  # the custom layout, not LinearLayoutManager
	recycler_view.set_prefetch_enabled(true)
	# 无需任何池/缓存配置：Recycler 的视图缓存容量会随布局可见数自动扩展
	# （对齐 Android Recycler.mViewCacheMax），大跳（scroll_to_position）时
	# 整个视口的 holder 都能进缓存、按类型复用，created 不涨。
	recycler_view.request_layout()
	%ScrollToButton.pressed.connect(_on_scroll_to_pressed)


func _process(_delta: float) -> void:
	var pooled := recycler_view.get_recycler().get_recycled_view_count(0) \
			+ recycler_view.get_recycler().get_cached_view_count()
	info_label.text = (
		"created %d（新视图；不再涨 = 回收-复用在工作） | 可见 %d | 缓存+池 %d\n"
		% [_adapter.created, recycler_view.get_child_holder_count(), pooled]
		+ "offset %d / %d | 滚到第 100 行按钮 → scroll_to_position(100)"
		% [recycler_view.get_scroll_offset(), recycler_view.get_layout().get_content_size(recycler_view) - int(recycler_view.get_viewport_size().y)]
	)


func _on_scroll_to_pressed() -> void:
	recycler_view.scroll_to_position(100)
