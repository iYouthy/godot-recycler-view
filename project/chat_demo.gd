extends Control

# 演示 reverse_layout 聊天列表：最新消息（position 0）在底部，滚轮上滑看历史。
# 布局方向反转（reverseLayout）后初始 scroll=0 就把 position 0 放在视口底部，
# 新消息 insert(0) 自动出现在底部——不需要 stack_from_end。
# 「发送」插入新消息并 smooth_scroll_to_position(0) 平滑回到底部（position 滚动）。

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var send_button: Button = %SendButton

var _adapter: ChatAdapter
var _messages: Array = []
var _seq := 0


class ChatAdapter extends Adapter:
	var messages: Array = []

	func _get_item_count() -> int:
		return messages.size()

	func _get_item_extent(_p: int) -> int:
		return 48

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var label := Label.new()
		label.add_theme_font_size_override("font_size", 14)
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(messages[position])


func _ready() -> void:
	for i in 80:
		_messages.append("消息 %03d" % _seq)
		_seq += 1
	_adapter = ChatAdapter.new()
	_adapter.messages = _messages
	recycler_view.set_item_extent(48)
	recycler_view.set_adapter(_adapter)
	var layout := LinearLayoutManager.new()
	layout.set_reverse_layout(true)
	recycler_view.set_layout(layout)
	send_button.pressed.connect(_send)


func _send() -> void:
	_messages.insert(0, "消息 %03d" % _seq)
	_seq += 1
	_adapter.notify_item_inserted(0)
	# position 0 = 最新消息，reverse 布局下它贴视口底部。
	recycler_view.smooth_scroll_to_position(0, 0.3)
