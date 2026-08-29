# 触摸交互与吸附

> 拖拽排序、滑动删除，以及把列表对齐到吸附目标。
> Demo：**item_touch_demo**、**snap_demo**。

## ItemTouchHelper —— 拖拽 & 滑动

把 `ItemTouchHelper` 与一个 `ItemTouchHelperCallback` 子类绑定。回调声明每个条目可被拖 / 滑的方向，
并把变化应用到你的数据。

```gdscript
class Callback extends ItemTouchHelperCallback:
    var adapter: Adapter

    func _get_movement_flags(holder: ViewHolder) -> int:
        return ItemTouchHelper.make_movement_flags(
            ItemTouchHelper.UP | ItemTouchHelper.DOWN,      # 拖拽方向
            ItemTouchHelper.LEFT | ItemTouchHelper.RIGHT)   # 滑动方向

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

# 设置：
var helper := ItemTouchHelper.new()
helper.set_callback(Callback.new())
helper.attach_to_recycler_view(recycler_view)
```

可覆写的钩子：

| 钩子 | 作用 |
|---|---|
| `_get_movement_flags(holder)` | 哪些方向可拖 / 可滑 |
| `_on_move(rv, dragged, target)` | 拖拽越过目标时交换数据并 notify |
| `_on_swiped(holder, direction)` | 滑动提交时删除数据并 notify |
| `_is_long_press_drag_enabled()` / `_is_item_view_swipe_enabled()` | 启用 / 停用手势 |
| `_get_swipe_threshold(holder)` / `_get_move_threshold(holder)` | 提交阈值（默认 0.5）|
| `_on_selected_changed` / `_clear_view` | 选中条目的视觉反馈 |

拖拽时 holder 的 Control 会钉在手指下；松手时要么换位要么弹回。被拖 / 滑的 holder 会排除出条目动画，
换位与删除淡出会自动与 `DefaultItemAnimator` 联动。

## SnapHelper —— 对齐到条目

挂上吸附助手，列表在 fling 与滚动停止后自动对齐。

**LinearSnapHelper** —— 最靠近视口中点的条目吸附到中心（chip 行、轮播）：

```gdscript
var snap := LinearSnapHelper.new()
snap.attach_to_recycler_view(chip_rv)
```

**PagerSnapHelper** —— 每次 fling 翻一整页；速度只决定方向，页面不跳过（分页 / 相册）：

```gdscript
var pager := PagerSnapHelper.new()
pager.attach_to_recycler_view(card_rv)
```

水平、垂直列表都支持吸附。Android 的三个吸附钩子（`findSnapView` 等）是 C++ 虚方法、脚本不可重写；
用内置的两种 helper，或写 C++ 子类。

## 下一步

- [滚动条](scroll_bars.md) —— 滚动时的视觉反馈。
- [条目动画](animations.md) —— 换位 / 删除如何动画。
