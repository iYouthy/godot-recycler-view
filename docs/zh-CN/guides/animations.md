# 条目动画

> 条目增删移动变化的淡入淡出与滑动动画。
> 实际运行：**recycler_demo**、**ops_demo**（点击编辑时变化会动画）。

## 默认动画器

`DefaultItemAnimator` 默认已挂载。收到更新时，RecyclerView 先跑两遍布局（记录每个条目的前后矩形），然后播放动画：

- **新增**条目淡入，
- **移除**条目淡出（淡出结束后被回收），
- **移动**条目从旧位置滑到新位置（也会跟随滚动），
- **变化**条目原地脉冲。

```gdscript
rv.set_item_animator(DefaultItemAnimator.new())
```

时长固定（移动 0.3s、增删 0.25s、变化 0.2s），GDScript 端不可调。

## 自定义动画器

想改时长或效果，继承 `ItemAnimator` 并覆写 `_animate_*` 虚方法。每个方法收到 holder 与前后矩形；
自己跑 tween，完成后记得把 holder 交还（通常 `rv.recycle_view`）以便复用。

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
    # _animate_move / _animate_change 同理

rv.set_item_animator(MyAnimator.new())
```

注意：

- 动画中的 holder 在被交还前不会被再次回收——过早（或从不）交还是常见 bug。用
  `RecyclerView.recycle_view` 交还。
- 自定义动画器与 `ItemTouchHelper` 配合良好；触摸助手会自动把拖拽 / 滑动中的 holder 排除出条目动画。

## 下一步

- [触摸交互与吸附](touch_interaction.md) —— 拖拽排序与条目动画联动。
- [数据更新与 DiffUtil](data_updates.md) —— 什么触发了动画。
