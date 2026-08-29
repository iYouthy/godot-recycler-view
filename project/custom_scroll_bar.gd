class_name CustomScrollBar
extends DefaultScrollBar

# 演示如何用 Godot 的 StyleBox 体系自定义滚动条——不需要重写 _draw。
# DefaultScrollBar 的 thumb/track 走 theme StyleBox（和内置 ScrollBar 的
# theme 项一致）：
#   "scroll"            = track 背景
#   "grabber"           = thumb 正常
#   "grabber_highlight" = thumb 悬停（自动切换，无需手动检测）
# 这里用 add_theme_stylebox_override() 设置；也可以用 Theme resource 统一管理。
# 宽度 16px（默认 8），胶囊圆角。


func _init() -> void:
	set_thickness(16.0)
	set_auto_hide(false)  # 常显方便观察


	var track := StyleBoxFlat.new()
	track.bg_color = Color(0.15, 0.15, 0.2, 0.2)
	track.set_corner_radius_all(8.0)

	var thumb := StyleBoxFlat.new()
	thumb.bg_color = Color(0.3, 0.65, 1.0, 0.9)
	thumb.set_corner_radius_all(8.0)

	var thumb_hover := StyleBoxFlat.new()
	thumb_hover.bg_color = Color(0.4, 0.75, 1.0, 0.95)
	thumb_hover.set_corner_radius_all(8.0)

	add_theme_stylebox_override("scroll", track)
	add_theme_stylebox_override("grabber", thumb)
	add_theme_stylebox_override("grabber_highlight", thumb_hover)
