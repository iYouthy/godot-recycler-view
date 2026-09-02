# 文档总览

[English overview](README.md)

godot-recycler-view 的文档。**类参考**已编译进 GDExtension 库（来源是 `doc_classes/*.xml`），由 Godot 编辑器直接展示——在脚本编辑器里按 **F1**（或悬停符号）即可查看描述、方法签名、成员与常量。下面的**教程**是带可运行代码的功能讲解。

## 教程

| 教程 | 主题 |
|---|---|
| [快速入门](zh-CN/guides/quick_start.md) | 最小垂直列表；回收复用原理 |
| [布局管理器](zh-CN/guides/layout_managers.md) | Linear / Grid / Staggered、方向、reverse 布局、水平 |
| [多视图类型与可变高度](zh-CN/guides/multi_view_types.md) | 多 view type、混合高度 |
| [数据更新与 DiffUtil](zh-CN/guides/data_updates.md) | notify_* 操作、ListAdapter + submit_list、payload 局部更新 |
| [条目动画](zh-CN/guides/animations.md) | DefaultItemAnimator、自定义动画器 |
| [触摸交互与吸附](zh-CN/guides/touch_interaction.md) | ItemTouchHelper 拖拽/滑动、SnapHelper |
| [滚动条](zh-CN/guides/scroll_bars.md) | 内置滚动条、四种模式、主题化 |
| [反向列表与嵌套](zh-CN/guides/reverse_and_nested.md) | 聊天布局、scroll_to_position、嵌套滚动 |

## 类参考

全部 33 个注册类都在编辑器文档里有一份条目。常用类：

- **核心** — RecyclerView · Adapter · ListAdapter · ViewHolder
- **布局** — LayoutManager · LinearLayoutManager · GridLayoutManager · StaggeredGridLayoutManager · SpanSizeLookup
- **装饰与动画** — ItemDecoration · ItemAnimator · DefaultItemAnimator
- **交互** — ItemTouchHelper · ItemTouchHelperCallback · SnapHelper · LinearSnapHelper · PagerSnapHelper · ScrollListener
- **滚动条** — 内置 ScrollBar · 滚动模式 · 自动隐藏
- **数据与 diff** — DiffUtil · DiffUtilCallback · DiffUtilItemCallback · DiffResult · ListUpdateCallback · BatchingListUpdateCallback · AdapterListUpdateCallback
- **内部机制** — Recycler · State · AdapterHelper · SortedList · SortedListCallback · AdapterDataObserver

每个类在编辑器里都会显示来自嵌入 XML 的完整描述、方法签名、成员与常量。上面的教程会结合代码讲解其中重要的部分。

## 构建与运行

构建、运行 demo、测试命令见 [README](../README.zh-CN.md)。
