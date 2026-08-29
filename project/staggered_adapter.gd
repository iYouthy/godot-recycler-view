extends Adapter

var items: Array = []
var created := 0


func _get_item_count() -> int:
	return items.size()


func _get_item_extent(position: int) -> int:
	# 瀑布流：item 高度不一，模拟图片卡片。
	return 60 + (position * 47) % 121  # 60..180


func _create_item(parent: Control, view_type: int) -> ViewHolder:
	created += 1
	var vh := ViewHolder.new()
	var rect := ColorRect.new()
	vh.set_control(rect)
	return vh


func _bind_item(holder: ViewHolder, position: int) -> void:
	var rect: ColorRect = holder.get_control()
	rect.color = Color.from_hsv(fmod(position * 0.13, 1.0), 0.55, 0.9)
