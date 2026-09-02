# 滚动条

> 滚动条使用 Godot 内置的 `VScrollBar` / `HScrollBar`（RecyclerView 内部子节点，随布局方向自动选择），
> 显示规则与"是否挤压内容"由 `scroll_mode` 控制。
> Demo：**horizontal_demo**（水平）、**custom_scroll_bar_demo**（主题化）、**auto_measure_scroll_bar_demo**（四种模式 + auto-measure 边界）。

## 四种模式

`vertical_scroll_mode` / `horizontal_scroll_mode` 决定该轴滚动条的显示与占位方式
（默认 `SCROLL_MODE_OVERLAY`）：

| 模式 | 挤压内容 | 不满一屏 | 超出一屏 |
|---|---|---|---|
| `SCROLL_MODE_OVERLAY` | 否（覆盖在内容上） | 不显示 | 显示 |
| `SCROLL_MODE_INSET` | 是（仅显示时） | 不显示 | 显示并让出空间 |
| `SCROLL_MODE_RESERVE` | 是（始终预留） | 不显示（空间保留） | 显示 |
| `SCROLL_MODE_NEVER_SHOW` | 否 | 永不显示（仍可滚动） | 永不显示 |

```gdscript
rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_INSET)
rv.set_horizontal_scroll_mode(RecyclerView.SCROLL_MODE_NEVER_SHOW)
```

Inset / Reserve 会把内容的可用宽度收窄（滚动条厚度，由主题决定）——因此**挤压会影响内容测量**：
`auto_measure_items` 下宽度敏感的内容（如换行文本）会在挤压生效后重排、行高变化，
RecyclerView 会自动清掉按旧宽度测量的缓存并重排（与窗口改宽同理）。

## 自动隐藏（淡出）

闲置淡出默认开启（Android 风格），对所有会显示的滚动条模式生效：

```gdscript
rv.set_scroll_bar_auto_hide(true)     # 默认
rv.set_scroll_bar_hide_delay(0.5)     # 淡出前等待秒数
```

内容跨过"超出一屏"的瞬间滚动条会**闪现一次**提示（Android 的 awakenScrollBars），再闲置淡出。

## 主题化

滚动条就是内置 `ScrollBar`，通过 `get_v_scroll_bar()` / `get_h_scroll_bar()` 取到后直接
`add_theme_stylebox_override`（theme 项与内置 ScrollBar 一致：`scroll` / `scroll_focus` 轨道，
`grabber` / `grabber_highlight` / `grabber_pressed` 滑块）：

```gdscript
var bar := rv.get_v_scroll_bar()
var track := StyleBoxFlat.new()
track.bg_color = Color(0.15, 0.15, 0.2, 0.2)
track.set_corner_radius_all(8.0)
track.content_margin_left = 4.0   # 注意：滚动条宽度 = track 的 minimum size，
track.content_margin_right = 4.0  # 自定义 stylebox 要带 content margins 才有宽度
var thumb := StyleBoxFlat.new()
thumb.bg_color = Color(0.3, 0.65, 1.0, 0.9)
thumb.set_corner_radius_all(8.0)
thumb.content_margin_left = 8.0
thumb.content_margin_right = 8.0
bar.add_theme_stylebox_override("scroll", track)
bar.add_theme_stylebox_override("grabber", thumb)
bar.add_theme_stylebox_override("grabber_highlight", thumb)
```

不需要子类——主题化即可覆盖外观；拖动、轨道点击翻页、键盘、焦点全部由内置 ScrollBar 处理。

## 实时信息

`scroll_horizontal` / `scroll_vertical`（检查器或 `set_h_scroll` / `set_v_scroll`）读写当前偏移；
`scroll_*_custom_step` 转发给滚动条（滚轮 / 箭头步长；拖动滑块本身按比例映射，不受步长影响）。

## 下一步

- [布局管理器](layout_managers.md) —— 滚动条轴向跟随布局方向。
- [触摸交互与吸附](touch_interaction.md) —— 吸附一个带滚动条的列表。
