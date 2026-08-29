# Multi view types & variable extents

> One list, several kinds of cells, each with its own scene, extent and recycled pool.
> Demos: **multi_type_demo**, **mixed_demo**.

## Multiple view types

Override `_get_item_view_type` to tag each position with a type. `_create_item` receives
that type, so you can instantiate a different cell per type. The Recycler keeps a separate
recycled pool per type, so scrolling still reuses holders — never a fresh one per pass.

```gdscript
class MyAdapter extends Adapter:
    var items: Array = []   # each item has a "type" field

    func _get_item_count() -> int:
        return items.size()

    func _get_item_view_type(position: int) -> int:
        return items[position]["type"]

    func _create_item(parent: Control, view_type: int) -> ViewHolder:
        var vh := ViewHolder.new()
        match view_type:
            0:
                vh.set_control(load("res://user_item.tscn").instantiate() as Control)
            1:
                vh.set_control(load("res://message_item.tscn").instantiate() as Control)
        return vh

    func _bind_item(holder: ViewHolder, position: int) -> void:
        var data: Dictionary = items[position]
        var item: Control = holder.get_control()
        if data["type"] == 0:
            (item.get_node("Name") as Label).text = data["name"]
        else:
            (item.get_node("Content") as Label).text = data["content"]
```

The view type also flows to `_bind_item` via `holder.get_item_view_type()`, which is handy
when one bind method serves several cell kinds.

## Variable extents

Pair `_get_item_view_type` with `_get_item_extent` so each cell kind sizes itself. Extents
only need to be per-position; the layout manager builds an offset table from them.

```gdscript
func _get_item_extent(position: int) -> int:
    return 72 if _get_item_view_type(position) == 0 else 48
```

Items that are taller or shorter than the row/grid slot are simply positioned by their
reported extent.

## Reuse accounting

Track how many fresh holders each type really created to confirm recycling is working (the
demos print this):

```gdscript
var created: Dictionary = {}

func _create_item(parent: Control, view_type: int) -> ViewHolder:
    created[view_type] = created.get(view_type, 0) + 1
    ...
```

Scrolling through a 1 000-item list should keep each `created[type]` bounded at roughly
"one screen worth".

## Next steps

- [Data updates & DiffUtil](data_updates.md) — updating the list efficiently.
- [Layout managers](layout_managers.md) — where the cells go.
