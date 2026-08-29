# Scroll bars

> Adding a scroll bar, theming it, and building your own.
> Demos: **horizontal_demo**, **custom_scroll_bar_demo**.

## The default scroll bar

Attach a `DefaultScrollBar` and it is bound as a child Control, pinned to the trailing edge,
with its axis picked from the layout direction (vertical for a vertical list, horizontal for
a horizontal one):

```gdscript
rv.set_scroll_bar(DefaultScrollBar.new())
```

It draws a track and a thumb sized by the viewport / content ratio, is draggable (or jumps
on track click), and fades out when the list is idle. Control auto-hide from the
RecyclerView:

```gdscript
rv.set_scroll_bar_auto_hide(true)     # default
rv.set_scroll_bar_hide_delay(0.5)     # seconds before fading
```

## Theming DefaultScrollBar

The bar uses the same theme items as Godot's built-in ScrollBar — `scroll` / `scroll_focus`
(track) and `grabber` / `grabber_highlight` (thumb). Override them with styleboxes, no
redraw code needed:

```gdscript
class CustomScrollBar extends DefaultScrollBar:
    func _init() -> void:
        set_thickness(16.0)
        set_auto_hide(false)                 # always visible, easier to observe

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

`track_color` / `thumb_color` / `corner_radius` are used as fallback when no theme stylebox
exists.

## A fully custom scroll bar

For a completely different widget, subclass the protocol base `RecyclerViewScrollBar`
(a `Control`). The RecyclerView calls `on_scroll_changed` on every scroll / layout and
exposes `get_offset()`, `get_viewport_size()` and `get_content_size()`. Draw in `_draw`
and handle dragging in `_gui_input`; override `_on_scroll_changed` to refresh.

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

`set_scroll_bar(null)` removes the bar; do not free the bar yourself — the RecyclerView owns
it.

## Next steps

- [Layout managers](layout_managers.md) — horizontal / reverse lists where the bar pins itself.
- [Touch interaction & snapping](touch_interaction.md) — snapping a list with a visible bar.
