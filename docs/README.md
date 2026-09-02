# Documentation

[中文总览](README.zh-CN.md)

godot-recycler-view documentation. The **class reference** is embedded in the GDExtension
library (from `doc_classes/*.xml`) and displayed by the Godot editor — press **F1** (or hover
a symbol) in the script editor to read descriptions, method signatures, members and
constants. The **guides** below are feature walkthroughs with runnable code.

## Guides

| Guide | Topics |
|---|---|
| [Quick start](en/guides/quick_start.md) | Minimal vertical list; how recycling works |
| [Layout managers](en/guides/layout_managers.md) | Linear / Grid / Staggered, orientation, reverse layout, horizontal |
| [Multi view types & variable heights](en/guides/multi_view_types.md) | Multiple view types, mixed heights |
| [Data updates & DiffUtil](en/guides/data_updates.md) | notify_* ops, ListAdapter + submit_list, payload partial updates |
| [Item animations](en/guides/animations.md) | DefaultItemAnimator, custom animators |
| [Touch interaction & snapping](en/guides/touch_interaction.md) | ItemTouchHelper drag / swipe, SnapHelper |
| [Scroll bars](en/guides/scroll_bars.md) | built-in bars, the four modes, theming |
| [Reverse lists & nesting](en/guides/reverse_and_nested.md) | Chat layouts, scroll_to_position, nested scrolling |

## Class reference

All 33 registered classes have an entry in the editor's documentation. The most used ones:

- **Core** — RecyclerView · Adapter · ListAdapter · ViewHolder
- **Layout** — LayoutManager · LinearLayoutManager · GridLayoutManager · StaggeredGridLayoutManager · SpanSizeLookup
- **Decoration & animation** — ItemDecoration · ItemAnimator · DefaultItemAnimator
- **Interaction** — ItemTouchHelper · ItemTouchHelperCallback · SnapHelper · LinearSnapHelper · PagerSnapHelper · ScrollListener
- **Scrolling** — built-in ScrollBars · scroll modes · auto-hide
- **Data & diff** — DiffUtil · DiffUtilCallback · DiffUtilItemCallback · DiffResult · ListUpdateCallback · BatchingListUpdateCallback · AdapterListUpdateCallback
- **Internal** — Recycler · State · AdapterHelper · SortedList · SortedListCallback · AdapterDataObserver

For each class the editor shows the full description, method signatures, members and
constants from the embedded XML. The guides above walk through the important ones with code.

## Setup

See the [README](../README.md) for building, running the demos, and the test commands.
