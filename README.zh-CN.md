# godot-recycler-view

Android [RecyclerView](https://developer.android.com/reference/androidx/recyclerview/widget/RecyclerView) 在 Godot 4 中的忠实移植，以 **GDExtension**（C++ / godot-cpp）形式提供。你在 GDScript 里就能得到一个虚拟化、可滚动的列表，并沿用 Android 的架构与术语：adapter、view holder、layout manager、回收池、item 动画等等。只有与视口相交的条目才会被实例化，所以即使列表有上万条，渲染出来的节点数也恒定。

```gdscript
var rv := RecyclerView.new()
rv.set_item_extent(40)
rv.set_adapter(MyAdapter.new())                 # 你的 Adapter 子类
rv.set_layout(LinearLayoutManager.new())        # 或 Grid / Staggered
add_child(rv)
```

文档：[English](docs/README.md) · [中文](docs/README.zh-CN.md)（类参考已编译进扩展，编辑器内直接可见）。

---

## 特性

核心列表：
- **RecyclerView** — 虚拟化、裁剪、三级视图复用（变更暂存区 → 按 position 的视图缓存 → 按类型的回收池）、预取。
- **Adapter / ListAdapter** — 必须实现 `_create_item` / `_bind_item` / `_get_item_count`；`ListAdapter.submit_list()` 自动 diff。
- **布局管理器** — `LinearLayoutManager`、`GridLayoutManager`（配合 `SpanSizeLookup`）、`StaggeredGridLayoutManager`（瀑布流），支持垂直/水平与 `reverse_layout`。
- **可变条目长度与多视图类型。**
- **`auto_measure_items` 内容自适应尺寸** — item 高度由控件内容决定(wrap_content 语义),`SIZE_EXPAND` 根节点占满视口(match_parent 语义);RichTextLabel + fit_content 即开即用。

数据更新：
- `notify_item_*` 增量更新（插入/移除/移动/变化），帧末统一应用。
- **DiffUtil** 支持稳定 id 与 **payload 局部更新**（只重绑变化的那一个子控件）。

交互与效果：
- **ItemAnimator / DefaultItemAnimator** — 增删淡入淡出、移动滑动动画。
- **ItemTouchHelper** — 长按拖拽排序与滑动删除。
- **SnapHelper** — `LinearSnapHelper`（居中吸附）与 `PagerSnapHelper`（一次 fling 翻一页）。
- **ScrollBar** — 可扩展的 `RecyclerViewScrollBar` 协议 + 可拖动、自动隐藏的 `DefaultScrollBar`（垂直/水平）。
- **ItemDecoration** — 条目内边距与绘制分隔线。
- **ScrollListener** — 滚动增量与 `IDLE / DRAGGING / SETTLING` 状态切换；fling 惯性滚动。
- **嵌套 RecyclerView** 滚动、聊天式 **reverse 布局**、`scroll_to_position` / `smooth_scroll_to_position`。

---

## 环境要求

- Godot **4.3+**（项目面向 4.7；编辑器文档需要 4.3+）。
- [SCons](https://scons.org/)、C++17 编译器、godot-cpp 子模块。

## 构建

```bash
git submodule update --init --recursive   # 拉取 godot-cpp
scons                                     # 构建并安装到 project/bin/<platform>/
```

构建时会把 `doc_classes/` 里的类参考编译进扩展库，编辑器即可显示每个类的文档。

`.gdextension` 描述文件在 `project/bin/godot_recycler_view.gdextension`；`project/` 是一个完整的 Godot 工程，可直接打开运行 demo。

## macOS GateKeeper

macOS 会对**从网络下载**（浏览器、GitHub Releases/Artifacts 等）的文件打上 quarantine 标记，并在首次加载时用 GateKeeper 校验签名：

- **本机编译**的 dylib（`scons` 产物）没有 quarantine 标记，可直接使用，不会被拦截。
- **CI（GitHub Actions）编译**的 dylib 属于"从网络下载"。CI 只做了 ad-hoc 签名（`codesign -s -`），它只能证明文件完整性，**不属于 Apple 开发者签名，也没有经过公证（notarization）**，因此 GateKeeper 仍会拦截：首次加载（运行 Godot 编辑器打开工程、或运行游戏）会弹出"无法验证开发者"之类的警告。
  - 解决：前往 **系统设置 → 隐私与安全性**，点击"仍要打开"/"允许"；或在该警告弹窗里选择打开方式。之后即可正常使用。

**如果你要分发基于本库开发的 macOS 版本**：需要申请 **Apple Developer 账号**，对编译产物（dylib 与 App）使用 **Developer ID 签名 + 公证（notarization）** 后才可以对外分发，否则用户下载后同样会被 GateKeeper 拦截。

## 快速上手

```gdscript
# list_adapter_demo.gd 展示了完整用法。
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

# 在某场景脚本中：
var rv := RecyclerView.new()
rv.set_item_extent(40)
rv.set_adapter(MyAdapter.new())
rv.set_layout(LinearLayoutManager.new())
rv.set_scroll_bar(DefaultScrollBar.new())      # 可选
add_child(rv)
```

数据变化通过 adapter 的 `notify_item_*` 方法通知；需要自动 diff 时用 `ListAdapter.submit_list()`。

## Demo

在 Godot 中打开 `project/` 目录并运行任意场景：

| 场景 | 演示内容 |
|---|---|
| `recycler_demo.tscn` | 一万条数据的垂直列表；实时随机编辑；created/visible 计数 |
| `list_adapter_demo.tscn` | `ListAdapter` + `submit_list()` 自动 diff |
| `multi_type_demo.tscn` | 单列表多视图类型 |
| `mixed_demo.tscn` | 混合视图类型 + 可变长度 |
| `grid_demo.tscn` | `GridLayoutManager` + `SpanSizeLookup` + 分隔线 |
| `staggered_demo.tscn` | `StaggeredGridLayoutManager` 瀑布流 |
| `ops_demo.tscn` | 插入/移除/移动/变化更新操作 |
| `partial_update_demo.tscn` | payload 局部更新（只重绑变化的单元格） |
| `diff_demo.tscn` | `DiffUtil` 最小操作 vs 全量重建对比 + 日志面板 |
| `item_touch_demo.tscn` | 拖拽排序与滑动删除 |
| `snap_demo.tscn` | `LinearSnapHelper` chip 行 + `PagerSnapHelper` 轮播 |
| `chat_demo.tscn` | reverse 布局聊天列表，新消息在底部 |
| `horizontal_demo.tscn` | 水平列表 + 水平滚动条 |
| `custom_scroll_bar_demo.tscn` | 继承 `RecyclerViewScrollBar` 的自定义滚动条 |
| `lifecycle_demo.tscn` | Adapter 生命周期回调（attach / detach / recycled / 拒绝回收）+ 实时事件日志 |
| `custom_layout_demo.tscn` | 在 GDScript 里继承 `LayoutManager` 自定义布局（波浪排布，含完整教学注释） |
| `scroll_jump_demo.tscn` | 观察 `scroll_to_position` 与 `smooth_scroll_to_position` —— 两种滚动都不应产生新的视图 |
| `rich_text_demo.tscn` | `auto_measure_items` 内容自适应尺寸：消息高度由文本决定，`SIZE_EXPAND` 系统公告占满视口，开关对比 |
| `nested_demo.tscn` | 嵌套 RecyclerView 联动滚动 |

## 文档

- **类参考** — 每个注册类一个 `doc_classes/*.xml`，已编译进扩展。在编辑器里按 F1（或悬停符号）即可查看 `RecyclerView`、`Adapter`、各布局管理器等方法签名、成员、常量与说明。
- **教程** — 带可运行代码的功能教程：
  - [中文教程](docs/zh-CN/guides/quick_start.md)
  - [English guides](docs/en/guides/quick_start.md)
- **总览** — [English](docs/README.md) · [中文](docs/README.zh-CN.md)。

## 测试

GDScript 测试（gdUnit4）：

```bash
cd project
godot --headless --path . -s addons/gdUnit4/bin/GdUnitCmdTool.gd --ignoreHeadlessMode -a res://test
```

纯算法层的独立 C++ 测试（不依赖 Godot 运行时）：

```bash
scons tests=yes && tests/bin/test_runner
```

## 与 Android 的差异

这是一个功能性移植，把 Android 的 API 适配到 Godot 的习惯用法，而非逐行克隆。显著差异及各自的处理方式：

- **回调而非监听器 / signal。** Android 通过注册在视图上的 Java 接口（`OnScrollListener`、`ItemTouchHelper.Callback`、`DiffUtil.ItemCallback`…）驱动行为；Godot 没有这套接口体系，因此本移植改用 **GDScript 虚方法**：继承并覆写 `_create_item`、`_bind_item`、`_get_item_count`、`_on_scrolled`、`_get_movement_flags`、`_are_items_the_same` 等。RecyclerView 本身不发出任何 Godot signal。
- **条目是 Control 而不是 View。** Android 的条目是带 `LayoutParams` 的 `View`，由 `LayoutManager` 测量并布局；这里条目是包在 `ViewHolder` 里的一个 `Control`，布局管理器用绝对矩形定位它，RecyclerView 裁剪到自己的视口。没有 measure/layout 遍历——通过 `_get_item_extent` / `set_item_extent` 给出条目尺寸即可。需要内容驱动尺寸时（Android 的 `wrap_content`）开启 `auto_measure_items`：测量挂钩在布局收口处，读取根控件的 combined minimum size（受 combined maximum size 约束），`SIZE_EXPAND` 根节点则取视口尺寸（Android 的 `match_parent`）；未测量的区域用 `item_extent` 作估计，测量后自动精化，`scroll_to_position` 会在实测后重锚目标。
- **布局与 diff 都是同步的。** Android 的 `requestLayout()` 延迟到下一次遍历，`ListAdapter.submitList()` 在后台线程 diff；这里 `request_layout()` 立即执行布局，`submit_list()` 同步 diff，同一帧内的调用立刻能看到新状态。`notify_item_*` 更新仍像 Android 一样在帧末批量应用。
- **统一的滚动空间。** Android 按像素滚动 `ViewGroup` 并按自己的约定上报 `dx`/`dy`；本移植保留相同的*内容偏移*模型（`get_scroll_offset`，clamp 到 `[0, content − viewport]`），但用自绘 clip + 偏移实现而非原生子节点滚动。手势、滚轮、fling、settle、ScrollBar、`ScrollListener` 全部共享这同一套空间，彼此之间无需换算。
- **复用以 position/type 为键，而非 stable id。** Android 的 `Recycler` 在位置变动时能通过 **stable id** 的 scrap 复用 holder；这里 stable id 会被记录（`has_stable_ids` / `get_item_id`），但视图缓存按精确位置、回收池按类型键控，复用发生在位置/类型层面而非 stable id。
- **用 `reverse_layout` 代替 `stackFromEnd`。** 聊天式"最新在底部"通过 `set_reverse_layout(true)` + 初始滚到末尾实现；Android 的 `stackFromEnd` 属性未移植。
- **更简化的预取与嵌套滚动。** Android 在滚动帧间运行 `GapWorker`，并通过 `NestedScrolling*` 接口协调父子；这里预取是直接的 `prefetch_view()` 单趟预热视口前方的池，嵌套滚动是较轻的接力——子列表先消费手势，剩余交给父列表。

### 尚未移植

- `StaggeredGridLayoutManager` 没有 gap strategy。
- `DefaultItemAnimator` 动画时长固定；自定义需继承 `ItemAnimator`。
- `SnapHelper` 的 Android 钩子是 C++ 虚方法，脚本不可重写（用 `LinearSnapHelper` / `PagerSnapHelper`）。
- 自定义 `LayoutManager` 走脚本虚方法（`_on_layout_children` 等，见 `custom_layout_demo`）；Android 的 `scrollVerticallyBy` / `scrollHorizontallyBy` 两个钩子未接入（滚动统一由 RecyclerView 的 offset 空间管理，布局只需响应 offset 变化）。
- state restoration（状态恢复）机制未移植；`AdapterDataObserver._on_state_restoration_policy_changed` 已注册但永不派发（没有 `set_state_restoration_policy` / 保存状态机制）。
- 仅支持 LTR；`START` / `END` 方向位映射为左 / 右。

## License

[The Unlicense](LICENSE.md) — 公有领域。
