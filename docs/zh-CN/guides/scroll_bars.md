# 滚动条

> 添加滚动条、美化它、以及完全自定义。
> Demo：**horizontal_demo**、**custom_scroll_bar_demo**。

## 默认滚动条

挂上 `DefaultScrollBar`，它就会被作为子 Control 绑定、钉到尾部边缘，并根据布局方向自动选择轴向
（垂直列表用垂直条、水平列表用水平条）：

```gdscript
rv.set_scroll_bar(DefaultScrollBar.new())
```

它会绘制轨道与按视口 / 内容比例缩放的滑块，可拖动（点轨道跳转），列表空闲时淡出。从 RecyclerView 控制自动隐藏：

```gdscript
rv.set_scroll_bar_auto_hide(true)     # 默认
rv.set_scroll_bar_hide_delay(0.5)     # 淡出前等待秒数
```

## 美化 DefaultScrollBar

滚动条与 Godot 内置 ScrollBar 使用相同的 theme 项——`scroll` / `scroll_focus`（轨道）与
`grabber` / `grabber_highlight`（滑块）。用 stylebox 覆盖即可，无需写绘制代码：

```gdscript
class CustomScrollBar extends DefaultScrollBar:
    func _init() -> void:
        set_thickness(16.0)
        set_auto_hide(false)                 # 常显方便观察

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

rv.set_scroll_bar(CustomScrollBar.new())
```

没有 theme stylebox 时用 `track_color` / `thumb_color` / `corner_radius` 兜底。

## 完全自定义滚动条

想要完全不同的控件，继承协议基类 `RecyclerViewScrollBar`（一个 `Control`）。RecyclerView 每次滚动 / 布局都会调用
`on_scroll_changed`，并通过 `get_offset()`、`get_viewport_size()`、`get_content_size()` 暴露数据。
在 `_draw` 里绘制、在 `_gui_input` 里处理拖动，覆写 `_on_scroll_changed` 刷新外观。

```gdscript
class MyBar extends RecyclerViewScrollBar:
    func _on_scroll_changed() -> void:
        queue_redraw()

    func _draw() -> void:
        var viewport := get_viewport_size()
        var content := get_content_size()
        if content <= viewport:
            return
        var thumb_h := float(viewport) / content * size.y
        var max_off := content - viewport
        var thumb_pos := float(get_offset()) / max_off * (size.y - thumb_h)
        draw_rect(Rect2(0, thumb_pos, size.x, thumb_h), Color(0.3, 0.65, 1.0, 0.9))
```

`set_scroll_bar(null)` 移除滚动条；不要自己释放滚动条——由 RecyclerView 管理。

## 下一步

- [布局管理器](layout_managers.md) —— 滚动条自动钉到尾部、自动选轴。
- [触摸交互与吸附](touch_interaction.md) —— 吸附一个带滚动条的列表。
