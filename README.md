# godot-recycler-view

A faithful port of Android's [RecyclerView](https://developer.android.com/reference/androidx/recyclerview/widget/RecyclerView) to Godot 4 as a **GDExtension**, written in C++ (godot-cpp). It gives you a virtualized, scrollable list in GDScript with the same architecture and terminology as the Android API: adapters, view holders, layout managers, recycled pools, item animations, and more. Only the items that intersect the viewport are instantiated, so even huge lists render with a constant number of nodes.

```gdscript
var rv := RecyclerView.new()
rv.set_item_size(40)
rv.set_adapter(MyAdapter.new())                 # your Adapter subclass
rv.set_layout(LinearLayoutManager.new())        # or Grid / Staggered
add_child(rv)
```

Documentation: [English](docs/README.md) · [中文](docs/README.zh-CN.md) (class reference is embedded in the extension and shown by the editor).

---

## Features

Core list:
- **RecyclerView** — virtualization, clipping, three-level view reuse (changed scrap → position-bound view cache → per-type recycled pool), prefetch.
- **Adapter / ListAdapter** — mandatory `_create_item` / `_bind_item` / `_get_item_count`; `ListAdapter.submit_list()` diffs automatically.
- **LayoutManagers** — `LinearLayoutManager`, `GridLayoutManager` (with `SpanSizeLookup`), `StaggeredGridLayoutManager` (masonry), vertical or horizontal, `reverse_layout`.
- **Variable item heights and multiple view types.**

Data updates:
- `notify_item_*` incremental updates (insert / remove / move / change), queued and applied at frame end.
- **DiffUtil** with stable ids and **payload partial updates** (only the changed child control is rebound).

Interaction & effects:
- **ItemAnimator / DefaultItemAnimator** — fade-in, fade-out, slide (move) animations on updates.
- **ItemTouchHelper** — long-press drag-to-reorder and swipe-to-dismiss.
- **SnapHelper** — `LinearSnapHelper` (center snap) and `PagerSnapHelper` (one page per fling).
- **ScrollBar** — extensible `RecyclerViewScrollBar` protocol + a draggable, auto-hiding `DefaultScrollBar` (vertical and horizontal).
- **ItemDecoration** — per-item insets and drawn separators (e.g. dividers).
- **ScrollListener** — scroll deltas and `IDLE / DRAGGING / SETTLING` state transitions; fling with inertial scrolling.
- **Nested RecyclerView** scrolling, chat-style **reverse layouts**, `scroll_to_position` / `smooth_scroll_to_position`.

---

## Requirements

- Godot **4.3+** (the project targets 4.7; editor docs need 4.3+).
- [SCons](https://scons.org/), a C++17 compiler, and the godot-cpp submodule.

## Building

```bash
git submodule update --init --recursive   # fetch godot-cpp
scons                                     # build + install into project/bin/<platform>/
```

The build compiles the class reference in `doc_classes/` into the extension library, so the editor shows the docs for every class (see below).

The `.gdextension` descriptor lives at `project/bin/godot_recycler_view.gdextension`; the `project/` folder is a complete Godot project you can open and run the demos from.

## Quick start

```gdscript
# list_adapter_demo.gd shows the full pattern.
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

# somewhere in a scene script:
var rv := RecyclerView.new()
rv.set_item_size(40)
rv.set_adapter(MyAdapter.new())
rv.set_layout(LinearLayoutManager.new())
rv.set_scroll_bar(DefaultScrollBar.new())      # optional
add_child(rv)
```

Data changes are reported through the adapter's `notify_item_*` methods; a diffing alternative is `ListAdapter.submit_list()`.

## Demos

Open the `project/` folder in Godot and run any scene:

| Scene | Shows |
|---|---|
| `recycler_demo.tscn` | Vertical list of 10k items; live random edits; created/visible counters |
| `list_adapter_demo.tscn` | `ListAdapter` + `submit_list()` auto-diffing |
| `multi_type_demo.tscn` | Multiple view types in one list |
| `mixed_demo.tscn` | Mixed view types with variable heights |
| `grid_demo.tscn` | `GridLayoutManager` with a `SpanSizeLookup` and dividers |
| `staggered_demo.tscn` | `StaggeredGridLayoutManager` masonry |
| `ops_demo.tscn` | Insert / remove / move / change update operations |
| `partial_update_demo.tscn` | Payload partial updates (only the changed cell rebinds) |
| `diff_demo.tscn` | `DiffUtil` minimal ops vs. a naive full rebuild, with a log panel |
| `animations_demo` | See `recycler_demo` (animations on) — add / remove / move slide & fade |
| `item_touch_demo.tscn` | Drag-to-reorder and swipe-to-dismiss |
| `snap_demo.tscn` | `LinearSnapHelper` chip row + `PagerSnapHelper` carousel |
| `chat_demo.tscn` | Reverse-layout chat list; new messages at the bottom |
| `horizontal_demo.tscn` | Horizontal list with a horizontal scroll bar |
| `custom_scroll_bar_demo.tscn` | A custom scroll bar subclassing `RecyclerViewScrollBar` |
| `nested_demo.tscn` | Nested RecyclerViews scrolling together |

## Documentation

- **Class reference** — one `doc_classes/*.xml` per registered class, compiled into the extension. In the editor, press F1 (or hover a symbol) to read descriptions, method signatures, members, and constants for `RecyclerView`, `Adapter`, the layout managers, etc.
- **Guides** — feature walkthroughs with runnable code:
  - [English guides](docs/en/guides/quick_start.md)
  - [中文教程](docs/zh-CN/guides/quick_start.md)
- **Overview** — [English](docs/README.md) · [中文](docs/README.zh-CN.md).

## Tests

GDScript tests (gdUnit4):

```bash
cd project
godot --headless --path . -s addons/gdUnit4/bin/GdUnitCmdTool.gd --ignoreHeadlessMode -a res://test
```

Standalone C++ tests for the pure-algorithm layer (no Godot runtime):

```bash
scons tests=yes && tests/bin/test_runner
```

## How it differs from Android

This is a functional port that adapts Android's API to Godot's idioms rather than a line-by-line
clone. The significant differences, and how each one is handled:

- **Callbacks, not listeners or signals.** Android wires behavior through Java interfaces
  (`OnScrollListener`, `ItemTouchHelper.Callback`, `DiffUtil.ItemCallback`, …) registered on the
  view. Godot has no such interface system, so this port uses **GDScript virtual methods**
  instead: subclass and override `_create_item`, `_bind_item`, `_get_item_count`, `_on_scrolled`,
  `_get_movement_flags`, `_are_items_the_same`, … The RecyclerView itself emits no Godot signals.
- **Items are Controls, not Views.** An Android item is a `View` with `LayoutParams`, measured and
  laid out by the `LayoutManager`. Here an item is a `Control` wrapped in a `ViewHolder`; the
  layout managers position it with absolute rects and the RecyclerView clips to its viewport.
  There is no measure/layout traversal — you give items a size through `_get_item_height` /
  `set_item_size`.
- **Synchronous layout and diffing.** Android's `requestLayout()` is deferred to the next
  traversal and `ListAdapter.submitList()` diffs on a background thread. Here `request_layout()`
  runs the layout immediately and `submit_list()` diffs synchronously, so a call in the same
  frame already sees the new state. `notify_item_*` updates are still batched and applied at
  frame end, like Android.
- **One shared scroll space.** Android scrolls a `ViewGroup` by pixels and reports `dx`/`dy` in
  its own conventions. This port keeps the same *content-offset* model
  (`get_scroll_offset`, clamped to `[0, content − viewport]`) but implements it with its own
  clip + offset rather than native child scrolling; gesture, wheel, fling, settle, ScrollBar and
  `ScrollListener` all share that single space, so there is no offset conversion between them.
- **Reuse is keyed by position/type, not stable id.** Android's `Recycler` can reuse a holder by
  **stable id** through its scrap even when positions shift. Stable ids are recorded here
  (`has_stable_ids` / `get_item_id`) but the view cache is position-exact and the recycled pool
  is type-keyed, so reuse happens on position/type rather than by stable id.
- **`reverse_layout` instead of `stackFromEnd`.** Chat-style "newest at the bottom" is done with
  `set_reverse_layout(true)` plus an initial scroll to the end; Android's `stackFromEnd` property
  is not ported.
- **Simpler prefetch and nested scrolling.** Android runs a `GapWorker` across scroll frames and
  coordinates parents/children through the `NestedScrolling*` interfaces. Here prefetch is a
  straightforward one-pass `prefetch_view()` that warms the pool just ahead of the viewport, and
  nested scrolling is a lighter relay where the child consumes the gesture first and hands the
  remainder to the parent.

### Not (yet) ported

- `StaggeredGridLayoutManager` has no gap strategy.
- `DefaultItemAnimator`'s durations are fixed; customize by subclassing `ItemAnimator`.
- `SnapHelper`'s Android hooks are C++ virtuals, not script-overridable (use `LinearSnapHelper` / `PagerSnapHelper`).
- `LayoutManager.get_content_size()` / `get_position_offset()` return their base default (0) from GDScript.
- `Adapter._on_item_recycled` and friends are declared but not yet invoked.
- Text is LTR only; `START` / `END` direction bits map to left / right.

## License

[The Unlicense](LICENSE.md) — public domain.
