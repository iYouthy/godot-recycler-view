extends Control

# Content-driven item sizes demo (RecyclerView.auto_measure_items).
#
# The item heights here are decided by the text, not by a static extent: each
# message is a RichTextLabel with fit_content, so its height is whatever the
# text needs at the list's width. Turn auto_measure_items off to see the same
# list clipped to the fixed item_extent — every message becomes 64px tall and
# the long ones get cut off.
#
# How it works (read the RecyclerView.auto_measure_items docs for the full
# contract):
#   - measurement hooks into set_item_view_position: every layout manager
#     (including scripted ones) funnels its fill through that call, the item
#     width is already fixed, and the item is inside the tree, so the theme
#     and fonts are correct. The root control's combined minimum size decides
#     the slot (Android's wrap_content), clamped by its combined maximum size
#     when one is declared.
#   - the first item is a "system banner": its root PanelContainer declares
#     SIZE_EXPAND along the scroll axis, so it spans the whole viewport like
#     Android's match_parent instead of hugging its content.
#   - measured heights are cached per position; any notify_* clears the cache
#     and the visible rows re-measure. Rows that have not been measured yet
#     use item_extent as an estimate, so a big jump may briefly land on the
#     estimate and then snap to the exact position (see the scroll buttons).
#
# What to look for: message heights differ per content and stay consistent
# while scrolling (created stops growing); toggling auto_measure off clips the
# long messages; "跳到底部" lands exactly on the last message.

const _ITEM_EXTENT_ESTIMATE := 64
const _LONG_TEXT := "这是一个相当长的消息:自动测量会读取 RichTextLabel 在布局宽度下换行后的实际内容高度,而不是使用静态的 item_extent。消息越长、换行越多,高度就越高,列表随之自动调整,行与行之间既不会重叠也不会留下空隙。"
const _SHORT_TEXT := "短消息"

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel
@onready var log_label: Label = %LogLabel
@onready var toggle_button: Button = %ToggleButton

var _adapter: _MessageAdapter
var _last_created := 0
var _log: Array[String] = []


# The item root is a VBoxContainer so the message can carry a header row; its
# height comes from the RichTextLabel's fit_content minimum size. The first
# position is the system banner: its root PanelContainer has SIZE_EXPAND on
# the vertical axis, so it fills the viewport instead of its content.
class _MessageAdapter extends Adapter:
	var messages: Array[String] = []
	var created := 0

	func _init(p_messages: Array[String]) -> void:
		messages = p_messages

	func _get_item_count() -> int:
		return messages.size()

	func _get_item_view_type(position: int) -> int:
		# Position 0 is the system banner, everything else a plain message.
		return 1 if position == 0 else 0

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		if view_type == 1:
			# System banner: SIZE_EXPAND -> the slot is the whole viewport.
			var banner := PanelContainer.new()
			banner.size_flags_vertical = Control.SIZE_EXPAND
			banner.add_theme_stylebox_override("panel", _make_banner_style())
			var label := Label.new()
			label.text = "系统公告(SIZE_EXPAND:占满整个视口)"
			label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			label.mouse_filter = Control.MOUSE_FILTER_PASS
			banner.add_child(label)
			vh.set_control(banner)
		else:
			var box := VBoxContainer.new()
			var header := Label.new()
			header.add_theme_font_size_override("font_size", 12)
			header.add_theme_color_override("font_color", Color(0.5, 0.5, 0.5))
			box.add_child(header)
			var rtl := RichTextLabel.new()
			rtl.mouse_filter = Control.MOUSE_FILTER_PASS
			rtl.set_fit_content(true)
			rtl.set_autowrap_mode(TextServer.AUTOWRAP_WORD_SMART)
			rtl.add_theme_font_size_override("normal_font_size", 18)
			box.add_child(rtl)
			vh.set_control(box)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		if position == 0:
			return
		var box := holder.get_control() as VBoxContainer
		(box.get_child(0) as Label).text = "消息 #%d" % position
		#var append = ""
		#for i in range(position * 5):
			#append += "append "
		#var msg := "%s %s" % [messages[position], append]
		(box.get_child(1) as RichTextLabel).set_text(messages[position])

	static func _make_banner_style() -> StyleBoxFlat:
		var sb := StyleBoxFlat.new()
		sb.bg_color = Color(0.1, 0.35, 0.7, 0.9)
		sb.corner_radius_top_left = 8
		sb.corner_radius_top_right = 8
		sb.corner_radius_bottom_left = 8
		sb.corner_radius_bottom_right = 8
		sb.content_margin_top = 12.0
		sb.content_margin_bottom = 12.0
		return sb


func _ready() -> void:
	_adapter = _MessageAdapter.new([_SHORT_TEXT, _LONG_TEXT, _SHORT_TEXT])
	for i in 20:
		_adapter.messages.append(_LONG_TEXT if i % 2 == 0 else _SHORT_TEXT)
	recycler_view.set_item_extent(_ITEM_EXTENT_ESTIMATE)
	recycler_view.set_auto_measure_items(true)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_scroll_bar(DefaultScrollBar.new())
	recycler_view.set_prefetch_enabled(false)
	recycler_view.request_layout()
	_last_created = _adapter.created
	_log.append("初始布局:内容高度决定行高,created %d" % _adapter.created)

	%ToggleButton.pressed.connect(_on_toggle)
	%AddButton.pressed.connect(_on_add)
	%GrowButton.pressed.connect(_on_grow)
	%BottomButton.pressed.connect(_on_bottom)


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	info_label.text = (
		"auto_measure_items:%s | created %d(操作增量 %+d)\n"
		% ["开" if recycler_view.get_auto_measure_items() else "关", _adapter.created, _adapter.created - _last_created]
		+ "可见 %d 项 | 内容总高 %dpx | 第 1 项实测高度 %dpx"
		% [recycler_view.get_child_holder_count(),
			recycler_view.get_layout().get_content_size(recycler_view),
			recycler_view.get_item_extent(1)]
	)
	var lines: Array[String] = ["── 操作记录 ──"]
	for l in _log.slice(maxi(_log.size() - 8, 0)):
		lines.append(l)
	log_label.text = "\n".join(lines)


func _record(op: String) -> void:
	_log.append("%s:created %+d" % [op, _adapter.created - _last_created])
	_last_created = _adapter.created


func _on_toggle() -> void:
	var enabled := not recycler_view.get_auto_measure_items()
	recycler_view.set_auto_measure_items(enabled)
	toggle_button.text = "关掉自动测量" if enabled else "开启自动测量"
	_record("切换 auto_measure → %s" % ("开" if enabled else "关"))


func _on_add() -> void:
	_adapter.messages.append(_SHORT_TEXT)
	_adapter.notify_item_inserted(_adapter.messages.size() - 1)
	_record("追加一条短消息")


func _on_grow() -> void:
	# 把某条消息替换为长文本:notify 清空测量缓存,该行重新测量变高,下面的
	# 行自动下移(见 _log 中"追加/变高"后的 created 增量——复用,不新建)。
	var idx := 1
	_adapter.messages[idx] = _LONG_TEXT + "追加的内容让这一条变得更高,下面的消息会整体往下移动。"
	_adapter.notify_item_changed(idx)
	_record("第 1 条消息变高")


func _on_bottom() -> void:
	# 大跳:目标区域的偏移先按估计值(item_extent=64)计算,布局测量后自动
	# 重锚到精确位置(参考 Android 的 mPendingScrollPosition)。
	recycler_view.scroll_to_position(_adapter.messages.size() - 1)
	_record("瞬间跳到底部(实测重锚)")
