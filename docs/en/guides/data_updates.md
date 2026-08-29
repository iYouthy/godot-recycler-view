# Data updates & DiffUtil

> The many ways to tell a list that its data changed — from manual `notify_*` calls to
> automatic diffing with `ListAdapter`, and payload partial updates.
> Demos: **ops_demo**, **list_adapter_demo**, **diff_demo**, **partial_update_demo**.

## Manual notify calls

After mutating your data, tell the RecyclerView exactly what changed. Calls are queued and
applied at frame end, so batch them freely.

```gdscript
adapter.items.append(new_item)
adapter.notify_item_inserted(adapter.items.size() - 1)

adapter.items.remove_at(3)
adapter.notify_item_removed(3)

adapter.items.move(2, 8)
adapter.notify_item_moved(2, 8)

adapter.items[5] = new_value
adapter.notify_item_changed(5)

adapter.items = brand_new_array
adapter.notify_data_changed()   # full rebuild, no animation
```

The range variants (`notify_item_range_inserted(0, 5)`, …) batch a block. `ops_demo`
drives insert / remove / move / change with buttons.

## ListAdapter — diff automatically

`ListAdapter` wraps a plain array and a `DiffUtilItemCallback`. `submit_list()` diffs the
current list against the new one and emits only the real changes — no manual wiring, no
`_get_item_count` override (it comes from the list).

```gdscript
class UserCallback extends DiffUtilItemCallback:
    func _are_items_the_same(old_item: Variant, new_item: Variant) -> bool:
        return old_item["id"] == new_item["id"]
    func _are_contents_the_same(old_item: Variant, new_item: Variant) -> bool:
        return old_item["name"] == new_item["name"]

class UserListAdapter extends ListAdapter:
    func _create_item(parent: Control, view_type: int) -> ViewHolder:
        var vh := ViewHolder.new()
        vh.set_control(Label.new())
        return vh
    func _bind_item(holder: ViewHolder, position: int) -> void:
        var user: Dictionary = get_item(position)
        (holder.get_control() as Label).text = user["name"]

# setup:
var adapter := UserListAdapter.new()
adapter.set_diff_callback(UserCallback.new())
rv.set_adapter(adapter)

# on data change:
adapter.submit_list(new_users)
```

`_are_items_the_same` decides identity (same id → a move or a change, not a remove+insert);
`_are_contents_the_same` decides whether a change op is needed at all. Renaming one row thus
emits a single `change`, not a rebuild — `created` stays flat (the demo shows this).

## DiffUtil — full control

When you need the diff yourself, use `DiffUtil.calculate_diff` with a position-based
`DiffUtilCallback`, then dispatch the result to a `ListUpdateCallback`:

```gdscript
class DiffCallback extends DiffUtilCallback:
    var old_items: Array = []
    var new_items: Array = []
    func _get_old_list_size() -> int: return old_items.size()
    func _get_new_list_size() -> int: return new_items.size()
    func _are_items_the_same(a: int, b: int) -> bool:
        return old_items[a]["id"] == new_items[b]["id"]
    func _are_contents_the_same(a: int, b: int) -> bool:
        return old_items[a]["text"] == new_items[b]["text"]

var callback := DiffCallback.new()
callback.old_items = old
callback.new_items = new_list
var diff := DiffUtil.calculate_diff(callback, true)
diff.dispatch_updates_to(update_callback)   # e.g. an AdapterListUpdateCallback
```

`diff_demo` prints the exact ops the diff emits (insert / remove / move / change) and
compares them against a naive full rebuild.

## Payload partial updates

When only part of a cell changed, `_get_change_payload` can say which part. The change op
then calls `_bind_item_with_payload`, which rebinds only the affected child controls —
the others keep their state.

```gdscript
class DiffCallback extends DiffUtilCallback:
    func _are_contents_the_same(a: int, b: int) -> bool:
        return old_items[a]["name"] == new_items[b]["name"] and old_items[a]["avatar"] == new_items[b]["avatar"]
    func _get_change_payload(a: int, b: int) -> Variant:
        var changes: Array[String] = []
        if old_items[a]["name"] != new_items[b]["name"]:
            changes.append("name")
        if old_items[a]["avatar"] != new_items[b]["avatar"]:
            changes.append("avatar")
        return changes

# in the adapter:
func _bind_item(holder: ViewHolder, position: int) -> void:
    # full rebind: set avatar AND name
    ...

func _bind_item_with_payload(holder: ViewHolder, position: int, payload: Variant) -> void:
    if payload is Array:
        for change in payload:
            if change == "name":
                (row.get_node("Name") as Label).text = ...
            elif change == "avatar":
                ...
        return
    _bind_item(holder, position)   # no payload → full rebind
```

`partial_update_demo` (each user row has an avatar + a name label) demonstrates updating
just one child control.

## Next steps

- [Item animations](animations.md) — the changes animate with DefaultItemAnimator.
- [Quick start](quick_start.md) — the basic wiring this guide builds on.
