class_name CustomScrollBarTheme

# 演示如何用 Godot 的 StyleBox 体系自定义 RV 的滚动条——不需要子类。
# RV 的滚动条就是 Godot 内置的 VScrollBar/HScrollBar（RV 构造时创建的
# 内部节点），通过 get_v_scroll_bar() / get_h_scroll_bar() 取到后直接
# 主题化即可：
#   bar.add_theme_stylebox_override("scroll", track)
#   bar.add_theme_stylebox_override("grabber", thumb)
# 主题项与内置 ScrollBar 完全一致：scroll / scroll_focus / grabber /
# grabber_highlight / grabber_pressed，外加 increment/decrement 箭头图标
# （不想要箭头可以用 Theme 覆盖成空纹理）。
# 注意：内置 bar 的宽度由主题 minimum size 决定（track 的 content margins
# + 箭头尺寸），所以自定义 track 要带 content margins，否则 bar 会变窄。


# 返回一个已应用 16px 胶囊样式的内置滚动条（直接改 RV 内部节点，无需返回值，
# 见 custom_scroll_bar_demo.gd 的用法）。
static func style(bar: ScrollBar) -> void:
	var track := StyleBoxFlat.new()
	track.bg_color = Color(0.15, 0.15, 0.2, 0.2)
	track.set_corner_radius_all(8.0)
	track.content_margin_left = 4.0
	track.content_margin_right = 4.0

	var thumb := StyleBoxFlat.new()
	thumb.bg_color = Color(0.3, 0.65, 1.0, 0.9)
	thumb.set_corner_radius_all(8.0)
	thumb.content_margin_left = 8.0
	thumb.content_margin_right = 8.0

	var thumb_hover := StyleBoxFlat.new()
	thumb_hover.bg_color = Color(0.4, 0.75, 1.0, 0.95)
	thumb_hover.set_corner_radius_all(8.0)
	thumb_hover.content_margin_left = 8.0
	thumb_hover.content_margin_right = 8.0

	bar.add_theme_stylebox_override("scroll", track)
	bar.add_theme_stylebox_override("grabber", thumb)
	bar.add_theme_stylebox_override("grabber_highlight", thumb_hover)
