# 多视图类型与可变长度

> 一个列表、多种单元格，每种有自己的场景、长度与回收池。
> Demo：**multi_type_demo**、**mixed_demo**。

## 多视图类型

覆写 `_get_item_view_type` 给每个位置打类型标签。`_create_item` 会收到该类型，据此实例化不同单元格。
Recycler 为每种类型维护独立的回收池，滚动时依然复用 holder——绝不会每一帧都新建。

```gdscript
class MyAdapter extends Adapter:
    var items: Array = []   # 每个 item 带 "type" 字段

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

视图类型也能通过 `holder.get_item_view_type()` 在 `_bind_item` 里拿到，适合一个 bind 方法服务多种单元格。

## 可变长度

把 `_get_item_view_type` 与 `_get_item_extent` 配对，让每种单元格自己决定长度。长度只需按位置返回，
布局管理器会根据它们构建偏移表。

```gdscript
func _get_item_extent(position: int) -> int:
    return 72 if _get_item_view_type(position) == 0 else 48
```

比格子高或矮的条目，按上报的长度定位即可。

## 复用统计

跟踪每种类型真正新建的 holder 数，确认回收生效（demo 会打印出来）：

```gdscript
var created: Dictionary = {}

func _create_item(parent: Control, view_type: int) -> ViewHolder:
    created[view_type] = created.get(view_type, 0) + 1
    ...
```

滚过 1000 条数据的列表时，每个 `created[type]` 应保持有界（约一屏的量）。

## 下一步

- [数据更新与 DiffUtil](data_updates.md) —— 高效更新列表。
- [布局管理器](layout_managers.md) —— 单元格放在哪里。
