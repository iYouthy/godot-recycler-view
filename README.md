# godot-recycler-view

A faithful port of Android's [RecyclerView](https://developer.android.com/reference/androidx/recyclerview/widget/RecyclerView) to Godot 4 as a **GDExtension**, written in C++ (godot-cpp). It gives you a virtualized, scrollable list in GDScript with the same architecture and terminology as the Android API: adapters, view holders, layout managers, recycled pools, item animations, and more. Only the items that intersect the viewport are instantiated, so even huge lists render with a constant number of nodes.

```gdscript
var rv := RecyclerView.new()
rv.set_item_extent(40)
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
- **Variable item extents and multiple view types.**
- **`auto_measure_items` content-sized items** — item heights come from the item control itself (wrap_content semantics); a `SIZE_EXPAND` root fills the viewport (match_parent). A `RichTextLabel` with fit_content works out of the box.

Data updates:
- `notify_item_*` incremental updates (insert / remove / move / change), queued and applied at frame end.
- **DiffUtil** with stable ids and **payload partial updates** (only the changed child control is rebound).

Interaction & effects:
- **ItemAnimator / DefaultItemAnimator** — fade-in, fade-out, slide (move) animations on updates.
- **ItemTouchHelper** — long-press drag-to-reorder and swipe-to-dismiss.
- **SnapHelper** — `LinearSnapHelper` (center snap) and `PagerSnapHelper` (one page per fling).
- **ScrollBar** — Godot built-in scroll bars (`Overlay` / `Inset` / `Reserve` / `Never Show` modes, auto-fade, theme directly).
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

## macOS GateKeeper

macOS tags files **downloaded from the network** (browser, GitHub Releases/Artifacts, ...) with a quarantine attribute and checks their signature with GateKeeper on first load:

- A dylib **built on your own machine** (`scons`) has no quarantine attribute and just works.
- A dylib **built by CI (GitHub Actions)** counts as downloaded. The CI applies only an *ad-hoc* signature (`codesign -s -`), which proves integrity but is **not an Apple Developer signature and is not notarized**, so GateKeeper still blocks it: the first load (opening the project in the Godot editor, or running the game) pops a "cannot verify the developer" style warning.
  - Workaround: go to **System Settings → Privacy & Security** and click "Open Anyway"/"Allow", or choose how to open it from the warning dialog. After that it works normally.

**If you ship a macOS build based on this library**: you need an **Apple Developer account** — sign the artifacts (dylib and app) with a **Developer ID certificate and notarize** them before distributing, otherwise your users hit the same GateKeeper block after downloading.

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
rv.set_item_extent(40)
rv.set_adapter(MyAdapter.new())
rv.set_layout(LinearLayoutManager.new())
rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_INSET)  # optional
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
| `mixed_demo.tscn` | Mixed view types with variable extents |
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
| `lifecycle_demo.tscn` | Adapter lifecycle callbacks (attach / detach / recycled / failed-to-recycle) with a live event log |
| `custom_scroll_bar_demo.tscn` | Theming the built-in scroll bar directly (16px capsule style) |
| `custom_layout_demo.tscn` | A GDScript-defined layout: subclass `LayoutManager` for a wave layout (full tutorial comments inside) |
| `scroll_jump_demo.tscn` | Observe `scroll_to_position` vs `smooth_scroll_to_position` — both must move without fabricating views |
| `rich_text_demo.tscn` | `auto_measure_items` content-sized items: message heights follow the text, a `SIZE_EXPAND` banner fills the viewport, toggle to compare |
| `auto_measure_scroll_bar_demo.tscn` | auto-measure + scroll bar boundary: append until the list overflows (bar flash hint) and switch the four modes live |
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
  There is no measure/layout traversal — you give items a size through `_get_item_extent` /
  `set_item_extent`. For content-driven sizes (Android's `wrap_content`), enable
  `auto_measure_items`: the measurement hooks into the layout funnel and reads the root
  control's combined minimum size (clamped by its combined maximum size); a `SIZE_EXPAND` root
  takes the viewport size instead (Android's `match_parent`). Unmeasured regions use
  `item_extent` as an estimate and refine as they are measured; `scroll_to_position`
  re-anchors to the exact target once the extents settle.
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
- Custom `LayoutManager`s are script virtuals (`_on_layout_children` etc., see `custom_layout_demo`); Android's `scrollVerticallyBy` / `scrollHorizontallyBy` hooks are not wired (scrolling lives in the RecyclerView's shared offset space — a layout only reacts to offset changes).
- State restoration is not ported; `AdapterDataObserver._on_state_restoration_policy_changed` is declared but never dispatched (there is no `set_state_restoration_policy` / saved-state mechanism).
- Text is LTR only; `START` / `END` direction bits map to left / right.

## License

[MIT](LICENSE.md) — permissive: use, modify, distribute and sell freely, with
attribution retained.

This project is a port of Android RecyclerView (androidx.recyclerview, AOSP,
Apache-2.0); the port is an original reimplementation and retains the upstream
attribution per the Apache License, as noted in the LICENSE file.
