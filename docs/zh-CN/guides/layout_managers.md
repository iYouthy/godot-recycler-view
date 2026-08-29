# 布局管理器

> 该选哪种布局、如何配置方向 / 列数、怎样做水平列表。
> Demo：**grid_demo**、**staggered_demo**、**horizontal_demo**、**mixed_demo**。

## LinearLayoutManager —— 一行 / 一列

默认的列表布局，支持可变条目高度。

```gdscript
var layout := LinearLayoutManager.new()
rv.set_layout(layout)
```

**水平**（一行 chip / 轮播）：

```gdscript
var layout := LinearLayoutManager.new()
layout.set_orientation(LinearLayoutManager.HORIZONTAL)
rv.set_layout(layout)
```

**反向**（条目从底部 / 尾部开始排 —— 聊天列表）：

```gdscript
layout.set_reverse_layout(true)
```

## GridLayoutManager —— 固定列数

条目从左到右流入 `span_count` 列；累计 span 满一行换行，行高取该行最高条目。

```gdscript
var layout := GridLayoutManager.new()
layout.set_span_count(3)
rv.set_layout(layout)
```

**通栏标题**通过 `SpanSizeLookup` 实现 —— 条目可跨多列：

```gdscript
class SpanLookup extends SpanSizeLookup:
    func _get_span_size(position: int) -> int:
        return 3 if position % 10 == 0 else 1   # 每第 10 项占满整行

var layout := GridLayoutManager.new()
layout.set_span_count(3)
layout.set_span_size_lookup(SpanLookup.new())
rv.set_layout(layout)
```

需要单元格几何信息时可用查询方法（`get_item_row`、`get_item_column`、`get_row_offset` 等）。

## StaggeredGridLayoutManager —— 瀑布流

每个新条目流入当前最短的列，各列独立增长，条目错开而非逐行对齐：

```gdscript
var layout := StaggeredGridLayoutManager.new()
layout.set_span_count(2)
rv.set_layout(layout)
```

## 一个列表里的可变高度

条目高度来自 `_get_item_height`（覆写时）或 `rv.set_item_size`。与视图类型配合可做混合信息流（见
[多视图类型](multi_view_types.md)）：

```gdscript
func _get_item_height(position: int) -> int:
    return 72 if items[position]["type"] == "user" else 48
```

## 装饰（分隔线）

`ItemDecoration` 负责条目内边距与绘制的分隔线。仓库里的 `divider_decoration.gd` 在每个条目下方画一条细线：

```gdscript
class Divider extends ItemDecoration:
    const GAP := 6
    func _get_item_offsets(position: int, parent: Control) -> Vector4:
        return Vector4(0, 0, 0, GAP)                    # 预留底部间距
    func _on_draw(parent: Control) -> void:
        for i in parent.get_child_holder_count():
            var rect := parent.get_decorated_item_rect(parent.get_child_holder_at(i).get_position())
            var y := rect.position.y + rect.size.y + GAP * 0.5
            parent.draw_line(Vector2(rect.position.x, y), Vector2(rect.position.x + rect.size.x, y), Color(0.6, 0.6, 0.6, 0.4), 1)

rv.add_item_decoration(Divider.new())
```

## 下一步

- [多视图类型](multi_view_types.md) —— 一个列表多种单元格。
- [反向列表与嵌套](reverse_and_nested.md) —— 聊天布局与嵌套滚动。
