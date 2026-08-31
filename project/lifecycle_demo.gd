extends Control

const LifecycleAdapterScript := preload("res://lifecycle_adapter.gd")

# Lifecycle demo: scroll through a 200-item list and watch the four Adapter
# lifecycle callbacks fire in real time — a log panel records every attach /
# detach / recycled / failed-to-recycle dispatch, with counters up top.
#
# What to look for:
#   - 滚出屏 → detach；滚回来 → attach，且视图上出现"复用×N"（同一视图被绑到
#     不同数据，回收-复用是虚拟化列表省内存的关键）。
#   - 滚动超过 1 个缓存位后 → recycled（视图数据被清、进池待复用）。
#   - 滚过 #40 顽固项（红色）：它 set_is_recyclable(false)，RV 每次布局都会咨询
#     _on_failed_to_recycle_view。默认"保留"：它钉在屏幕上不走（ghost）；
#     点"强制回收"开关后，下一次滚过它就正常回收。
#   - prefetch 预创建的视图直接进池，不产生 recycled 事件（created 涨、recycled
#     不涨），滚到时才复用。

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel
@onready var log_label: Label = %LogLabel
@onready var force_button: Button = %ForceButton

var _adapter: LifecycleAdapterScript


func _ready() -> void:
	_adapter = LifecycleAdapterScript.new()
	recycler_view.set_item_extent(40)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_prefetch_enabled(true)
	recycler_view.request_layout()

	force_button.pressed.connect(_on_force_pressed)
	%ResetButton.pressed.connect(_on_reset_pressed)
	_update_info()


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	var pooled := recycler_view.get_recycler().get_recycled_view_count(0) \
			+ recycler_view.get_recycler().get_cached_view_count()
	info_label.text = (
		"created %d（新视图，不再涨 = 回收在起作用） | attach %d | detach %d | recycled %d | 拒绝 %d\n"
		% [_adapter.created, _adapter.attached, _adapter.detached, _adapter.recycled, _adapter.failed]
		+ "可见 %d | 缓存+池 %d（等待复用的视图） | #40 顽固项：%s"
		% [recycler_view.get_child_holder_count(), pooled, "强制回收" if _adapter.force else "保留（钉住不走）"]
	)
	var lines: Array[String] = ["── 最近事件 ──"]
	for e in _adapter.events.slice(maxi(_adapter.events.size() - 12, 0)):
		lines.append(e)
	log_label.text = "\n".join(lines)


func _on_force_pressed() -> void:
	_adapter.force = not _adapter.force
	force_button.text = "顽固项 #40：%s" % ("强制回收" if _adapter.force else "保留")
	_adapter.events.append("策略切换 → %s" % ("强制回收" if _adapter.force else "保留"))


func _on_reset_pressed() -> void:
	recycler_view.scroll_to_position(0)
	_adapter.attached = 0
	_adapter.detached = 0
	_adapter.recycled = 0
	_adapter.failed = 0
	_adapter.events.clear()
