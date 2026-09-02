# Scroll bars

> Scroll bars are Godot's built-in `VScrollBar` / `HScrollBar` (internal children of the
> RecyclerView, axis picked from the layout direction). Whether they show and whether they
> take layout space is driven by the `scroll_mode` properties.
> Demos: **horizontal_demo** (horizontal), **custom_scroll_bar_demo** (theming),
> **auto_measure_scroll_bar_demo** (all four modes + auto-measure boundary).

## The four modes

`vertical_scroll_mode` / `horizontal_scroll_mode` control the bar's display and space-taking
for that axis (default `SCROLL_MODE_OVERLAY`):

| Mode | Takes space | Content fits | Content overflows |
|---|---|---|---|
| `SCROLL_MODE_OVERLAY` | No (drawn over the items) | Hidden | Shown |
| `SCROLL_MODE_INSET` | Yes (while shown) | Hidden | Shown, items laid out clear of it |
| `SCROLL_MODE_RESERVE` | Yes (always) | Hidden (space kept) | Shown |
| `SCROLL_MODE_NEVER_SHOW` | No | Never shown (the RV still scrolls) | Never shown |

```gdscript
rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_INSET)
rv.set_horizontal_scroll_mode(RecyclerView.SCROLL_MODE_NEVER_SHOW)
```

Inset / Reserve narrow the content's usable width by the bar's thickness (theme-driven), so
**carving affects content measurement**: under `auto_measure_items`, width-sensitive content
(such as wrapped text) re-flows and changes height once the carve kicks in. The RecyclerView
drops the measurements taken at the old width and relayouts automatically (same path as a
window resize).

## Auto-hide (fade)

Fading out when idle is on by default (Android-style) and applies to every mode that shows a bar:

```gdscript
rv.set_scroll_bar_auto_hide(true)     # default
rv.set_scroll_bar_hide_delay(0.5)     # seconds to wait before fading out
```

When the content crosses the "overflows one screen" threshold, the bar **flashes once** as a
hint (Android's awakenScrollBars), then fades out when idle.

## Theming

The bars are built-in `ScrollBar`s: grab them with `get_v_scroll_bar()` / `get_h_scroll_bar()`
and style with `add_theme_stylebox_override` (same theme items as the built-in ScrollBar:
`scroll` / `scroll_focus` track, `grabber` / `grabber_highlight` / `grabber_pressed` thumb):

```gdscript
var bar := rv.get_v_scroll_bar()
var track := StyleBoxFlat.new()
track.bg_color = Color(0.15, 0.15, 0.2, 0.2)
track.set_corner_radius_all(8.0)
track.content_margin_left = 4.0   # note: the bar's width comes from the track's
track.content_margin_right = 4.0  # minimum size — give custom styleboxes content margins
var thumb := StyleBoxFlat.new()
thumb.bg_color = Color(0.3, 0.65, 1.0, 0.9)
thumb.set_corner_radius_all(8.0)
thumb.content_margin_left = 8.0
thumb.content_margin_right = 8.0
bar.add_theme_stylebox_override("scroll", track)
bar.add_theme_stylebox_override("grabber", thumb)
bar.add_theme_stylebox_override("grabber_highlight", thumb)
```

No subclassing needed — theming covers the look; dragging, track-click paging, keyboard and
focus are all handled by the built-in ScrollBar.

## Live state

`scroll_horizontal` / `scroll_vertical` (inspector, or `set_h_scroll` / `set_v_scroll`) read and
write the current offset; `scroll_*_custom_step` forwards to the bar (wheel/arrow step; dragging
the thumb is ratio-mapped and ignores the step).

## Next steps

- [Layout managers](layout_managers.md) — the bar's axis follows the layout direction.
- [Touch interaction & snapping](touch_interaction.md) — snapping on a list with a bar.
