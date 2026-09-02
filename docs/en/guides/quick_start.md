# Quick start

> A minimal vertical list, then a short explanation of how the recycling works.
> Run it live: open `project/` and play the **recycler_demo** scene.

## The four pieces

A RecyclerView list is built from four cooperating objects:

1. **Adapter** — knows the item count and how to create / bind item views.
2. **ViewHolder** — a light wrapper around the `Control` that displays one item.
3. **LayoutManager** — decides where items go (`LinearLayoutManager`, `GridLayoutManager`, `StaggeredGridLayoutManager`).
4. **RecyclerView** — the `Control` you put in your scene; it does the layout and the recycling.

## Minimal example

```gdscript
class MyAdapter extends Adapter:
    var items: Array = []

    func _get_item_count() -> int:
        return items.size()

    func _create_item(parent: Control, view_type: int) -> ViewHolder:
        var vh := ViewHolder.new()
        var label := Label.new()
        label.set_size(Vector2(200, 40))
        vh.set_control(label)
        return vh

    func _bind_item(holder: ViewHolder, position: int) -> void:
        (holder.get_control() as Label).text = str(items[position])


# in a scene script:
var rv := RecyclerView.new()
rv.set_item_extent(40)                # default item extent along the scroll axis
rv.set_adapter(MyAdapter.new())
rv.set_layout(LinearLayoutManager.new())
rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_OVERLAY)  # scroll bar mode (optional; Overlay is the default)
add_child(rv)
```

The three mandatory overrides are `_get_item_count`, `_create_item` and `_bind_item`.
`set_item_extent` sets the default extent for all items; override `_get_item_extent` in the
adapter when items have different extents.

## Changing data

Mutate your data array, then tell the RecyclerView with a `notify_*` call:

```gdscript
adapter.items.append(new_item)
adapter.notify_item_inserted(adapter.items.size() - 1)

adapter.items.remove_at(3)
adapter.notify_item_removed(3)

adapter.items[5] = updated_value
adapter.notify_item_changed(5)
```

Updates are queued and applied at the end of the frame, so you can batch several `notify_*`
calls in one frame. If you mutate everything, `notify_data_changed()` rebuilds the whole
list. For large lists prefer [ListAdapter](data_updates.md), which diffs two lists and only
emits the real changes.

## How recycling works

`recycler_demo` shows the result: 10 000 items render with only a handful of holders
instantiated (`created` stays bounded as you scroll). Internally the Recycler keeps three
pools:

| Pool | Keyed by | Purpose |
|---|---|---|
| View cache | exact layout position | fastest reuse when you scroll back to a just-left position |
| Recycled pool | view type | reuse any same-type holder when scrolling far |
| Changed scrap | position, within one layout cycle | holds holders displaced by an update until layout finishes |

When a new position scrolls into view the Recycler checks the cache, then the pool, then
asks the adapter to create a fresh holder. When an item scrolls out it goes back to the
cache or pool. Only the ~one-screen worth of holders exists at any time.

## Next steps

- [Layout managers](layout_managers.md) — grids, staggered masonry, horizontal and reverse lists.
- [Multi view types](multi_view_types.md) — different cells in one list.
- [Data updates & DiffUtil](data_updates.md) — ListAdapter, payload partial updates.
