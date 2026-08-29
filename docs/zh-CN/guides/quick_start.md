# 快速入门

> 最小垂直列表，以及回收复用原理的简要说明。
> 实际运行：打开 `project/` 并播放 **recycler_demo** 场景。

## 四个组成部分

一个 RecyclerView 列表由四个协作对象组成：

1. **Adapter** —— 知道条目数量，以及如何创建 / 绑定条目视图。
2. **ViewHolder** —— 包装"显示一个条目的 Control"的轻量对象。
3. **LayoutManager** —— 决定条目放哪里（`LinearLayoutManager`、`GridLayoutManager`、`StaggeredGridLayoutManager`）。
4. **RecyclerView** —— 放进场景的 `Control`，负责布局与回收。

## 最小示例

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


# 场景脚本里：
var rv := RecyclerView.new()
rv.set_item_size(40)                # 沿滚动轴的默认条目尺寸
rv.set_adapter(MyAdapter.new())
rv.set_layout(LinearLayoutManager.new())
rv.set_scroll_bar(DefaultScrollBar.new())   # 可选
add_child(rv)
```

三个必须实现的虚方法是 `_get_item_count`、`_create_item`、`_bind_item`。
`set_item_size` 设置所有条目的默认高度；条目高度不一致时在 adapter 里覆写 `_get_item_height`。

## 修改数据

改完数据数组后，用 `notify_*` 通知 RecyclerView：

```gdscript
adapter.items.append(new_item)
adapter.notify_item_inserted(adapter.items.size() - 1)

adapter.items.remove_at(3)
adapter.notify_item_removed(3)

adapter.items[5] = updated_value
adapter.notify_item_changed(5)
```

更新会排队并在帧末统一应用，所以一帧内可以连发多个 `notify_*`。整体重排用
`notify_data_changed()`。大数据量列表推荐用 [ListAdapter](data_updates.md)，它会 diff 两份列表、
只发出真正的变化。

## 回收复用原理

`recycler_demo` 展示了结果：10000 条数据渲染时只实例化了少量 holder（滚动时 `created`
保持有界）。内部 Recycler 维护三个池：

| 池 | 以什么为键 | 用途 |
|---|---|---|
| 视图缓存 | 精确布局位置 | 滚回刚离开的位置时最快复用 |
| 回收池 | 视图类型 | 远距离滚动时复用任意同类型 holder |
| 变更暂存区 | 位置（单个布局周期内） | 更新中被顶出的 holder 在布局结束前暂存 |

新位置滚进视口时，Recycler 依次查缓存、查池，最后才让 adapter 新建。滚出的条目回到缓存或池。
任何时刻只存在约一屏的 holder。

## 下一步

- [布局管理器](layout_managers.md) —— 网格、瀑布流、水平与反向列表。
- [多视图类型](multi_view_types.md) —— 一个列表里多种单元格。
- [数据更新与 DiffUtil](data_updates.md) —— ListAdapter、payload 局部更新。
