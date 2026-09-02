extends Control

# auto-measure + 滚动条边界 demo：从不满一屏追加到超出一屏，观察四种
# scroll_mode 的行为差异（内容高度由行高实测决定，宽度变化会改变文本换行，
# 所以 Inset 挤压生效时行高/内容高度会跟着变——这正是观察重点）。
#
# 用法：
#   「追加一项」跨过一屏边界 → bar 出现（闪现一次提示，然后闲置淡出）
#   「切换模式」循环 Overlay / Inset / Reserve / Never Show
#   「auto-hide」开关淡出行为（on = 闲置 0.5s 淡出；off = 常显）
#
# 状态行含义：
#   内容高/视口高 = 是否溢出（溢出才显示 bar）
#   bar 可见/alpha = Overlay 与 Inset 显示规则相同；Inset/Reserve 会把视口
#   挤窄（item0 宽 < RV 宽），Overlay/Never Show 不挤。

class ItemCallback extends DiffUtilItemCallback:
	func _are_items_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["id"] == new_item["id"]

	func _are_contents_the_same(old_item: Variant, new_item: Variant) -> bool:
		return old_item["text"] == new_item["text"]


class ItemAdapter extends ListAdapter:
	var created := 0

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.size_flags_vertical = Control.SIZE_EXPAND_FILL
		label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		# 宽度敏感内容：可用宽度变窄时文本换行变多、行变高。auto-measure
		# 按实测行高布局，所以 Inset/Reserve 挤压视口后内容高度会上升。
		label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		var box := PanelContainer.new()
		box.add_child(label)
		vh.set_control(box)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var item: Dictionary = get_item(position)
		var box: PanelContainer = holder.get_control()
		var stylebox := StyleBoxFlat.new()
		stylebox.bg_color = Color.from_hsv(fmod(position * 0.07, 1.0), 0.35, 0.55)
		stylebox.set_corner_radius_all(12.0)
		stylebox.content_margin_left = 12.0
		stylebox.content_margin_right = 12.0
		stylebox.content_margin_top = 8.0
		stylebox.content_margin_bottom = 8.0
		box.add_theme_stylebox_override("panel", stylebox)
		var label: Label = box.get_child(0)
		var append_text := ""
		for i in range(position):
			append_text += "append "
		label.text = "%s %s" %[item["text"], append_text]


const _MODE_NAMES := ["Overlay", "Inset", "Reserve", "Never Show"]

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel
@onready var mode_button: Button = %ModeButton
@onready var auto_hide_button: Button = %AutoHideButton

var _adapter: ItemAdapter
var _seq := 0
var _mode := 0

const _LONG := "这一项是长文本：可用宽度变窄时换行变多、行高变高，内容总高度也随之变化 —— Inset / Reserve 挤压视口后能直观看到行高变化与内容高度重估。"
const _SHORT := "短文本项。"


func _ready() -> void:
	_adapter = ItemAdapter.new()
	_adapter.set_diff_callback(ItemCallback.new())
	recycler_view.set_item_extent(40)  # 未测量前的估计行高（auto-measure 实测后修正）
	recycler_view.set_auto_measure_items(true)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	%AppendButton.pressed.connect(_on_append_pressed)
	%RemoveButton.pressed.connect(_on_remove_pressed)
	%ModeButton.pressed.connect(_on_mode_pressed)
	%AutoHideButton.pressed.connect(_on_auto_hide_pressed)
	# 初始 3 项：不满一屏，滚动条不显示。
	_adapter.submit_list(_make_items(3))
	_apply_mode()


func _make_items(n: int) -> Array:
	var arr := []
	for i in n:
		_seq += 1
		arr.append({
			"id": _seq,
			"text": ("%d. " % _seq) + (_LONG if _seq % 3 == 0 else _SHORT + "编号 %d" % _seq),
		})
	return arr


func _on_append_pressed() -> void:
	var arr := _adapter.get_current_list().duplicate(true)
	arr.append_array(_make_items(1))
	_adapter.submit_list(arr)


func _on_remove_pressed() -> void:
	var arr := _adapter.get_current_list().duplicate(true)
	if not arr.is_empty():
		arr.pop_back()
		_adapter.submit_list(arr)


func _on_mode_pressed() -> void:
	_mode = (_mode + 1) % 4
	_apply_mode()


func _apply_mode() -> void:
	recycler_view.set_vertical_scroll_mode(_mode)
	mode_button.text = "模式：%s（点击切换）" % _MODE_NAMES[_mode]


func _on_auto_hide_pressed() -> void:
	var on := not recycler_view.get_scroll_bar_auto_hide()
	recycler_view.set_scroll_bar_auto_hide(on)
	auto_hide_button.text = "auto-hide：%s" % ("on" if on else "off")


func _process(_delta: float) -> void:
	var bar: ScrollBar = recycler_view.get_v_scroll_bar()
	var content_h := 0
	var viewport := recycler_view.get_viewport_size()
	if recycler_view.get_layout() != null:
		content_h = recycler_view.get_layout().get_content_size(recycler_view)
	var first_w := -1.0
	if recycler_view.get_child_holder_count() > 0:
		first_w = recycler_view.get_child_holder_at(0).get_control().size.x
	var bar_state := "不可见"
	if bar.is_visible():
		bar_state = "可见 alpha=%.2f" % bar.get_modulate().a
	info_label.text = "模式 %s | 内容高 %d / 视口高 %d（%s）| bar：%s | item0 宽 %.0f / RV 宽 %.0f | created %d" % [
		_MODE_NAMES[_mode], content_h, int(viewport.y),
		"已溢出" if content_h > viewport.y else "不满一屏",
		bar_state, first_w, recycler_view.get_size().x, _adapter.created,
	]
