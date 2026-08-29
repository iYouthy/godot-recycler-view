# Layout managers

> Which layout to pick, how to configure orientation / spans, and how to go horizontal.
> Demos: **grid_demo**, **staggered_demo**, **horizontal_demo**, **mixed_demo**.

## LinearLayoutManager — one item per row / column

The default list layout, with variable item heights.

```gdscript
var layout := LinearLayoutManager.new()
rv.set_layout(layout)
```

**Horizontal** (a row of chips / a carousel):

```gdscript
var layout := LinearLayoutManager.new()
layout.set_orientation(LinearLayoutManager.HORIZONTAL)
rv.set_layout(layout)
```

**Reverse** (items start from the bottom / trailing edge — chat lists):

```gdscript
layout.set_reverse_layout(true)
```

## GridLayoutManager — fixed columns

Items flow left-to-right into `span_count` columns; a line ends when the accumulated span
fills it, and its height is the tallest item in the line.

```gdscript
var layout := GridLayoutManager.new()
layout.set_span_count(3)
rv.set_layout(layout)
```

**Full-width headers** via a `SpanSizeLookup` — items can span several columns:

```gdscript
class SpanLookup extends SpanSizeLookup:
    func _get_span_size(position: int) -> int:
        return 3 if position % 10 == 0 else 1   # every 10th item spans the whole row

var layout := GridLayoutManager.new()
layout.set_span_count(3)
layout.set_span_size_lookup(SpanLookup.new())
rv.set_layout(layout)
```

Helper queries (`get_item_row`, `get_item_column`, `get_row_offset`, …) are available if you
need cell geometry from a script.

## StaggeredGridLayoutManager — masonry

Each new item flows into the currently shortest column, so columns grow independently and
items are staggered instead of row-aligned:

```gdscript
var layout := StaggeredGridLayoutManager.new()
layout.set_span_count(2)
rv.set_layout(layout)
```

## Variable heights in one list

Item heights come from `_get_item_height` when overridden, otherwise from
`rv.set_item_size`. Combine it with view types for a mixed feed (see
[multi_view_types](multi_view_types.md)):

```gdscript
func _get_item_height(position: int) -> int:
    return 72 if items[position]["type"] == "user" else 48
```

## Decorations (dividers)

`ItemDecoration` adds insets and drawn separators. The repo's `divider_decoration.gd` draws
a thin line under every item:

```gdscript
class Divider extends ItemDecoration:
    const GAP := 6
    func _get_item_offsets(position: int, parent: Control) -> Vector4:
        return Vector4(0, 0, 0, GAP)                    # reserve a bottom gap
    func _on_draw(parent: Control) -> void:
        for i in parent.get_child_holder_count():
            var rect := parent.get_decorated_item_rect(parent.get_child_holder_at(i).get_position())
            var y := rect.position.y + rect.size.y + GAP * 0.5
            parent.draw_line(Vector2(rect.position.x, y), Vector2(rect.position.x + rect.size.x, y), Color(0.6, 0.6, 0.6, 0.4), 1)

rv.add_item_decoration(Divider.new())
```

## Next steps

- [Multi view types](multi_view_types.md) — several cell kinds in one list.
- [Reverse lists & nesting](reverse_and_nested.md) — chat layouts and nested scrolling.
