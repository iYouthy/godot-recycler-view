# 反向列表与嵌套

> 从底部开始的聊天式列表、按 position 滚动，以及嵌套 RecyclerView。
> Demo：**chat_demo**、**nested_demo**。

## 反向布局（聊天列表）

`set_reverse_layout(true)` 让条目从尾部开始排——垂直列表里第一项在底部。配合默认滚动偏移，
聊天列表一进来就显示最新消息在底部，无需手动滚动。

```gdscript
var layout := LinearLayoutManager.new()
layout.set_reverse_layout(true)
rv.set_layout(layout)

# 新消息：插入到 position 0，即正好在底部
messages.insert(0, "new message")
adapter.notify_item_inserted(0)
```

新消息 `insert(0)` 自动出现在底部；滚轮上滑看更早的历史（符合聊天直觉）。

## 按 position 滚动

跳到某个条目，或平滑滚过去：

```gdscript
rv.scroll_to_position(42)                 # 瞬时；顶部对齐（reverse 时底部对齐）
rv.smooth_scroll_to_position(42, 0.3)     # 0.3 秒缓动
```

典型的"发送"处理把最新消息滚进视野：

```gdscript
func _send() -> void:
    messages.insert(0, "new message")
    _seq += 1
    adapter.notify_item_inserted(0)
    rv.smooth_scroll_to_position(0, 0.3)   # position 0 = 最新 = 底部
```

## 嵌套 RecyclerView

RecyclerView 可以互相嵌套——垂直方向嵌套（垂直 feed 里塞一个水平 chip 行）或同方向嵌套
（垂直子列表，滚动接力）。在外层 adapter 的 `_create_item` 里构建内层 RecyclerView：

```gdscript
class NestedAdapter extends Adapter:
    func _get_item_view_type(position: int) -> int:
        if position == 0:
            return 1                       # chip 行（水平子列表）
        if position == 1:
            return 2                       # 垂直子列表
        return 0

    func _get_item_height(position: int) -> int:
        if position == 0:
            return 64
        if position == 1:
            return 320
        return 48

    func _create_item(parent: Control, view_type: int) -> ViewHolder:
        var vh := ViewHolder.new()
        if view_type == 1:
            var root := Control.new()
            root.clip_contents = true      # 让 chip 行留在条目内缩矩形里
            root.set_size(Vector2(360, 64))
            var chips := RecyclerView.new()
            chips.set_size(Vector2(360, 64))
            chips.set_item_size(48)
            chips.set_adapter(ChipAdapter.new())
            var layout := LinearLayoutManager.new()
            layout.set_orientation(LinearLayoutManager.HORIZONTAL)
            chips.set_layout(layout)
            root.add_child(chips)
            vh.set_control(root)
        elif view_type == 2:
            # ... 一个装在自己裁剪 Control 里的垂直子列表
        else:
            vh.set_control(Label.new())
        return vh
```

给条目的根 `Control` 开 `clip_contents`，让嵌套列表留在内缩矩形里；内层 RecyclerView 接好线后调用
`request_layout()`。同方向嵌套会接力滚动：内层先消费手势，剩余部分交给外层。

## 下一步

- [快速入门](quick_start.md) —— 本教程依赖的 adapter 基础。
- [布局管理器](layout_managers.md) —— chip 行用到的水平布局。
