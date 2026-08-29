# 数据更新与 DiffUtil

> 通知数据变化的多种方式——从手写 `notify_*` 到 `ListAdapter` 自动 diff，再到 payload 局部更新。
> Demo：**ops_demo**、**list_adapter_demo**、**diff_demo**、**partial_update_demo**。

## 手动 notify

改完数据后精确告诉 RecyclerView 哪里变了。调用会排队并在帧末应用，可以随便批量。

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
adapter.notify_data_changed()   # 全量重建，无动画
```

区间变体（`notify_item_range_inserted(0, 5)` 等）批量处理一段。`ops_demo` 用按钮驱动插入 / 移除 / 移动 / 变化。

## ListAdapter —— 自动 diff

`ListAdapter` 包装一个普通数组和一个 `DiffUtilItemCallback`。`submit_list()` 会对当前列表与新列表做 diff，
只发出真正变化——无需手动接线，也无需覆写 `_get_item_count`（由列表提供）。

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

# 设置：
var adapter := UserListAdapter.new()
adapter.set_diff_callback(UserCallback.new())
rv.set_adapter(adapter)

# 数据变化时：
adapter.submit_list(new_users)
```

`_are_items_the_same` 判断身份（同一 id → 移动或修改，而不是删除+插入）；`_are_contents_the_same`
判断是否需要发出 change。给一行改名因此只发一个 change，而不是重建——`created` 保持不动（demo 有展示）。

## DiffUtil —— 完全控制

需要自己拿 diff 时，用 `DiffUtil.calculate_diff` 配合按位置的 `DiffUtilCallback`，再把结果派发给
`ListUpdateCallback`：

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
diff.dispatch_updates_to(update_callback)   # 例如 AdapterListUpdateCallback
```

`diff_demo` 会打印 diff 发出的精确操作（插入 / 删除 / 移动 / 修改），并与全量重建对比。

## Payload 局部更新

单元格只有部分变化时，`_get_change_payload` 可以说出变了哪部分。change 操作随后调用
`_bind_item_with_payload`，只重绑受影响的子控件——其他子控件保持状态。

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

# adapter 里：
func _bind_item(holder: ViewHolder, position: int) -> void:
    # 全量重绑：头像和名字都设
    ...

func _bind_item_with_payload(holder: ViewHolder, position: int, payload: Variant) -> void:
    if payload is Array:
        for change in payload:
            if change == "name":
                (row.get_node("Name") as Label).text = ...
            elif change == "avatar":
                ...
        return
    _bind_item(holder, position)   # 无 payload → 全量重绑
```

`partial_update_demo`（每行用户 = 头像 + 名字两个子控件）演示了只更新一个子控件。

## 下一步

- [条目动画](animations.md) —— 这些变化由 DefaultItemAnimator 带动画。
- [快速入门](quick_start.md) —— 本教程依赖的基础接线。
