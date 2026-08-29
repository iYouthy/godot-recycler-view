# Reverse lists & nesting

> Chat-style lists that start at the bottom, position scrolling, and nested RecyclerViews.
> Demos: **chat_demo**, **nested_demo**.

## Reverse layout (chat lists)

`set_reverse_layout(true)` lays items out from the trailing end, so with a vertical list the
first item sits at the bottom. Combined with the default scroll offset this already shows a
chat's newest message at the bottom — no manual scrolling required.

```gdscript
var layout := LinearLayoutManager.new()
layout.set_reverse_layout(true)
rv.set_layout(layout)

# new message: it is inserted at position 0, i.e. right at the bottom
messages.insert(0, "new message")
adapter.notify_item_inserted(0)
```

New messages `insert(0)` appear at the bottom automatically; a wheel scroll upward goes
back in history (matches the chat intuition).

## Position scrolling

Jump to a specific item, or glide there smoothly:

```gdscript
rv.scroll_to_position(42)                 # instant; top-aligned (bottom-aligned in reverse)
rv.smooth_scroll_to_position(42, 0.3)     # eased over 0.3s
```

A typical "send" handler scrolls the latest message into view:

```gdscript
func _send() -> void:
    messages.insert(0, "new message")
    _seq += 1
    adapter.notify_item_inserted(0)
    rv.smooth_scroll_to_position(0, 0.3)   # position 0 = newest = bottom
```

## Nested RecyclerViews

RecyclerViews can nest — perpendicular (a horizontal chip row inside a vertical feed) or
same-direction (a vertical sub-list with scroll relay). Build the inner RecyclerView in the
outer adapter's `_create_item`:

```gdscript
class NestedAdapter extends Adapter:
    func _get_item_view_type(position: int) -> int:
        if position == 0:
            return 1                       # chip row (horizontal child)
        if position == 1:
            return 2                       # vertical sub-list
        return 0

    func _get_item_height(position: int) -> int:
        if position == 0:
            return 64
        if position == 1:
            return 320
        return 48

    func _create_item(parent: Control, view_type: int) -> ViewHolder:
        var vh := ViewHolder.new()
        if view_type == 1:
            var root := Control.new()
            root.clip_contents = true      # keep the chip row inside the item's inset rect
            root.set_size(Vector2(360, 64))
            var chips := RecyclerView.new()
            chips.set_size(Vector2(360, 64))
            chips.set_item_size(48)
            chips.set_adapter(ChipAdapter.new())
            var layout := LinearLayoutManager.new()
            layout.set_orientation(LinearLayoutManager.HORIZONTAL)
            chips.set_layout(layout)
            root.add_child(chips)
            vh.set_control(root)
        elif view_type == 2:
            # ... a vertical sub-list in its own clipped Control
        else:
            vh.set_control(Label.new())
        return vh
```

Use `clip_contents` on the item's root `Control` so a nested list stays inside its inset
rect, and call `request_layout()` on inner RecyclerViews after wiring them up. Same-direction
nesting relays the scroll so the inner list consumes the gesture first, then hands the
remainder to the outer one.

## Next steps

- [Quick start](quick_start.md) — the adapter basics this builds on.
- [Layout managers](layout_managers.md) — horizontal layouts used by the chip row.
