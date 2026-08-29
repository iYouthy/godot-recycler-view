# Touch interaction & snapping

> Drag-to-reorder, swipe-to-dismiss, and aligning the list to a snapped item.
> Demos: **item_touch_demo**, **snap_demo**.

## ItemTouchHelper — drag & swipe

Attach an `ItemTouchHelper` with an `ItemTouchHelperCallback` subclass. The callback
declares which directions each item may be dragged / swiped, and applies the change to your
data.

```gdscript
class Callback extends ItemTouchHelperCallback:
    var adapter: Adapter

    func _get_movement_flags(holder: ViewHolder) -> int:
        return ItemTouchHelper.make_movement_flags(
            ItemTouchHelper.UP | ItemTouchHelper.DOWN,      # drag directions
            ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT)   # swipe directions

    func _on_move(recycler_view: Object, dragged: ViewHolder, target: ViewHolder) -> bool:
        adapter.items.move(dragged.get_position(), target.get_position())
        adapter.notify_item_moved(dragged.get_position(), target.get_position())
        return true

    func _on_swiped(holder: ViewHolder, direction: int) -> void:
        adapter.items.remove_at(holder.get_position())
        adapter.notify_item_removed(holder.get_position())

    func _on_selected_changed(holder: ViewHolder, action_state: int) -> void:
        if action_state == ItemTouchHelper.ACTION_STATE_DRAG:
            holder.get_control().modulate = Color(0.8, 0.9, 1.0)

    func _clear_view(holder: ViewHolder) -> void:
        holder.get_control().modulate = Color.WHITE

# setup:
var helper := ItemTouchHelper.new()
helper.set_callback(Callback.new())
helper.attach_to_recycler_view(recycler_view)
```

What to override:

| Hook | Purpose |
|---|---|
| `_get_movement_flags(holder)` | which directions drag / swipe work |
| `_on_move(rv, dragged, target)` | swap data and notify when a drag crosses a target |
| `_on_swiped(holder, direction)` | delete data and notify when a swipe commits |
| `_is_long_press_drag_enabled()` / `_is_item_view_swipe_enabled()` | enable / disable gestures |
| `_get_swipe_threshold(holder)` / `_get_move_threshold(holder)` | commit thresholds (default 0.5) |
| `_on_selected_changed` / `_clear_view` | visual feedback for the selected holder |

During a drag the holder's Control is pinned under the finger; on release it either swaps
into place or snaps back. Dragged / swiped holders are excluded from item animations, and
the reorder / delete fades integrate with `DefaultItemAnimator` automatically.

## SnapHelper — align to an item

Attach a snap helper and the list aligns itself after flings and scroll stops.

**LinearSnapHelper** — the item nearest the viewport's center snaps to the center (chip rows,
carousels):

```gdscript
var snap := LinearSnapHelper.new()
snap.attach_to_recycler_view(chip_rv)
```

**PagerSnapHelper** — one full page per fling; velocity only picks the direction, pages
never skip (a pager / photo gallery):

```gdscript
var pager := PagerSnapHelper.new()
pager.attach_to_recycler_view(card_rv)
```

Snapping works on horizontal and vertical lists. The three Android snap hooks
(`findSnapView`, …) are C++ virtuals and not script-overridable; use the two built-in
helpers or write a C++ subclass.

## Next steps

- [Scroll bars](scroll_bars.md) — visual feedback while scrolling.
- [Item animations](animations.md) — how reorder / delete animate.
