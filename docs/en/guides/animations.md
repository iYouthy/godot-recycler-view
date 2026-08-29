# Item animations

> Fade and slide animations for item add / remove / move / change.
> Run it live: **recycler_demo** and **ops_demo** (edits animate as you click).

## The default animator

`DefaultItemAnimator` is attached by default. When an update arrives, the RecyclerView runs
a two-pass layout (recording each item's rect before and after), then animates:

- **added** items fade in,
- **removed** items fade out (and are recycled when the fade finishes),
- **moved** items slide from their old position to the new one (they also follow scrolling),
- **changed** items pulse in place.

```gdscript
rv.set_item_animator(DefaultItemAnimator.new())
```

The durations are fixed (move 0.3 s, add / remove 0.25 s, change 0.2 s) and cannot be tuned
from GDScript.

## Custom animator

To change timing or effect, subclass `ItemAnimator` and override the `_animate_*` virtuals.
Each receives the holder and the pre/post layout rects; animate your tween and make sure the
holder is released once done (usually via `rv.recycle_view`) so it can be reused.

```gdscript
class MyAnimator extends ItemAnimator:
    func _animate_add(holder: ViewHolder, from: Rect2, to: Rect2) -> void:
        var control := holder.get_control()
        var tween := control.create_tween()
        control.modulate.a = 0.0
        tween.tween_property(control, "modulate:a", 1.0, 0.6)
        tween.finished.connect(func():
            rv.recycle_view(holder, holder.get_position()))

    func _animate_remove(holder: ViewHolder, from: Rect2, to: Rect2) -> void:
        var control := holder.get_control()
        var tween := control.create_tween()
        tween.tween_property(control, "modulate:a", 0.0, 0.6)
        tween.finished.connect(func():
            rv.recycle_view(holder, holder.get_position()))
    # _animate_move / _animate_change similarly

rv.set_item_animator(MyAnimator.new())
```

Notes:

- Holders that are animating are excluded from further recycling until released — releasing
  too early (or never) is the usual bug. Release via `RecyclerView.recycle_view`.
- Custom animators play well with `ItemTouchHelper`; the touch helper excludes dragged /
  swiped holders from item animations automatically.

## Next steps

- [Touch interaction & snapping](touch_interaction.md) — drag reorder animates with the item animator.
- [Data updates & DiffUtil](data_updates.md) — what triggers the animations.
