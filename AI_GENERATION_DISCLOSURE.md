# AI Generation & Usage Disclosure

**Project**: godot-recycler-view — a faithful port of Android's RecyclerView to Godot 4,
shipped as a GDExtension (C++ / godot-cpp).
**Maintainer**: 江毅 (youthyjj@gmail.com)
**Purpose of this document**: to describe, as required by the Godot Asset Library review,
*what* was AI-generated in this asset and *how* AI was used in its creation.

---

## English

### 1. Summary

This asset was developed through a **human-directed, AI-implemented workflow**. The
overwhelming majority of the code, tests, demo scenes, build configuration, CI workflow and
documentation in this repository was **written by AI** (the tools in §2). The maintainer did
not author the source files directly; his role was **product owner and reviewer**:
specifying each requirement, reviewing and approving the design and plan *before*
implementation, running acceptance demos, and reporting issues until quality was accepted.
Every feature shipped in this asset was therefore human-supervised, human-tested and
human-approved before release.

### 2. Tools used

- **Claude Code** (Anthropic's agentic coding tool / CLI), running the **DeepSeek V4 Flash**
  language model — performed the entire development loop: research, planning, implementation,
  testing and documentation.
- **Godot 4.x** (editor + engine source) and **godot-cpp** — the target platform and the
  GDExtension binding framework.
- **gdUnit4** — GDScript unit-test framework; **SCons** — build system.

### 3. What was AI-generated, and how it was reviewed

| Artifact | AI role | Human role |
|---|---|---|
| Core C++ GDExtension source (`src/`): RecyclerView engine, layout managers, adapters, view-holder recycling/pooling, prefetch, DiffUtil, item animations, ItemTouchHelper, SnapHelpers, scroll bars, ItemDecoration | Generated | Requirements; plan review/approval; acceptance through demos; bug reports |
| GDScript test suite (gdUnit4) and standalone C++ algorithm tests | Generated | Test results reviewed; regressions reported and traced |
| Demo scenes and scripts (`project/*_demo.tscn/.gd`) | Generated | **Each demo run and verified by hand** (scrolling, gestures, animations, nested lists, …) |
| Documentation: guides (`docs/`), class reference (`doc_classes/`), README (EN + zh-CN) | Generated | Edited and approved; factual mistakes reported and fixed |
| SCons build scripts, GitHub Actions CI workflow, `.gdextension` descriptor | Generated | Approved; CI artifacts verified on macOS / Windows / Linux |
| This disclosure document | AI-assisted | Facts supplied and approved by the maintainer |

### 4. How AI was used — the development process

The project was built feature by feature through a strict, repeatable loop, with
**test-driven development (TDD)** followed throughout:

1. **Requirement** — the maintainer stated a feature request, usually mirroring a specific
   Android RecyclerView behavior (e.g. "support an Inset-style scroll bar like Android's").
2. **Research & plan** — the AI researched feasibility against the reference materials in §6
   and proposed an execution plan. The maintainer reviewed it and could amend or reject it
   before any code was written.
3. **Implementation (TDD)** — only after plan approval did the AI implement: failing tests
   written first, then code until the suite passed. Pure-algorithm layers (DiffUtil, layout
   math, adapter bookkeeping) are covered by standalone C++ tests; everything else by gdUnit4
   tests running in a headless Godot instance.
4. **Acceptance demo** — for user-visible features the AI wrote a runnable demo scene
   (e.g. `auto_measure_scroll_bar_demo.tscn`, `chat_demo.tscn`) exercising the feature.
5. **Acceptance & feedback loop** — the maintainer ran the demo in the Godot editor, verified
   the behavior, and reported problems (in Chinese). The AI diagnosed and fixed each report —
   adding or updating tests where needed — until the maintainer accepted the result. Only
   then was the feature considered done and committed.

This loop ran for every feature in the asset; at no stage did AI write code that was not
subsequently exercised by automated tests and by the maintainer's hands-on verification.

### 5. Verification

- **Automated** — 231 gdUnit4 tests, 137 documentation tests, and standalone C++ algorithm
  tests; the full suite passes at the project's latest revision. The maintainer re-ran the
  suite before release.
- **Manual** — the maintainer interactively verified every demo in the Godot editor
  (scrolling, touch/drag interaction, animations, nested lists, scroll-bar behavior, …).
- **Traceable** — the repository's git history records the per-feature development: each
  step of the loop above is visible as commits, so AI output and human fixes are auditable.

### 6. Reference materials provided to the AI

To guarantee API and behavioral fidelity to the upstream, the AI was given:

- the **full AndroidX RecyclerView source code** (AOSP, androidx.recyclerview, Apache-2.0)
  as the behavioral specification to port;
- the **Godot 4 engine source code** and API documentation, so the code conforms to Godot
  conventions and the godot-cpp / GDExtension binding layer;
- the relevant **Android official documentation** for the ported APIs.

### 7. Originality & licensing

The implementation is an **original reimplementation** written from the Android API surface
under the guidance above — it is not copied Android source. The asset is released under the
**MIT** license; per the Apache License, the upstream AOSP RecyclerView attribution is
retained in `LICENSE.md`. All AI-generated code in this asset is contributed by its author
(the maintainer) under the same MIT license.

---

## 中文

### 1. 概述

本资源以"**人类主导方向、AI 实施开发**"的方式完成。仓库中绝大部分代码、测试、演示
场景、构建配置、CI 工作流与文档均由 **AI 编写**（工具见 §2）。作者本人不直接撰写源
文件，他的角色是**需求方与验收人**：逐个提出需求，在动手实现*之前*审核并批准设计与
方案，运行验收 Demo，并持续反馈问题直至质量达标。因此，本资源发布的每一个特性在面世
前都经过人的监督、测试与验收。

### 2. 使用的工具

- **Claude Code**（Anthropic 的智能编码工具 / CLI），运行 **DeepSeek V4 Flash** 模型——
  承担了从调研、方案、实现、测试到文档的完整开发闭环。
- **Godot 4.x**（编辑器与引擎源码）与 **godot-cpp** —— 目标平台与 GDExtension 绑定框架。
- **gdUnit4** —— GDScript 单元测试框架；**SCons** —— 构建系统。

### 3. 哪些内容由 AI 生成，以及如何被人工审核

| 产物 | AI 的角色 | 人的角色 |
|---|---|---|
| 核心 C++ GDExtension 源码（`src/`）：RecyclerView 引擎、布局管理器、Adapter、ViewHolder 回收与池化、预取、DiffUtil、item 动画、ItemTouchHelper、SnapHelper、滚动条、ItemDecoration | 生成 | 提需求；审核/批准方案；Demo 验收；反馈 bug |
| GDScript 测试套件（gdUnit4）与独立 C++ 算法测试 | 生成 | 审阅测试结果；报告并定位回归 |
| Demo 场景与脚本（`project/*_demo.tscn/.gd`） | 生成 | **逐个手动运行验收**（滚动、手势、动画、嵌套列表等） |
| 文档：教程（`docs/`）、类参考（`doc_classes/`）、README（中英双语） | 生成 | 修改与批准；事实性错误被反馈并修复 |
| SCons 构建脚本、GitHub Actions CI 工作流、`.gdextension` 描述文件 | 生成 | 批准；CI 产物在 macOS / Windows / Linux 上验证 |
| 本说明文件 | AI 辅助 | 事实由作者提供并确认 |

### 4. AI 的使用方式 —— 开发流程

本项目按特性逐个开发，采用严格、可复现的循环，全程遵循**测试驱动开发（TDD）**：

1. **提需求** —— 作者陈述功能需求，通常是复刻某一种 Android RecyclerView 的行为
   （例如"支持 Android 那样的 Inset 风格滚动条"）。
2. **调研与方案** —— AI 对照 §6 的参考资料调研可行性并给出执行计划；作者审核，可在
   动手前提出修改或否决。
3. **实施（TDD）** —— 方案批准后 AI 才开始写代码：先写失败用例，再实现到测试全绿。
   纯算法层（DiffUtil、布局计算、adapter 簿记）由独立 C++ 测试覆盖，其余由 gdUnit4
   在无头 Godot 实例中覆盖。
4. **验收 Demo** —— 对用户可见的功能，AI 编写可运行的演示场景（如
   `auto_measure_scroll_bar_demo.tscn`、`chat_demo.tscn`）来实际操作该功能。
5. **验收与反馈循环** —— 作者在 Godot 编辑器中运行 Demo、人工核验行为并反馈问题（中文）；
   AI 逐个定位并修复——需要时同步补/改测试——直至作者验收通过。特性至此才算完成并提交。

每个特性都完整走过上述循环；任何阶段都不存在"AI 写了代码但未经自动化测试或作者亲手
验证"的情况。

### 5. 验证方式

- **自动化** —— 231 个 gdUnit4 测试、137 个文档测试及独立 C++ 算法测试，项目最新修订版
  全套通过；发布前作者再次运行确认。
- **人工** —— 作者在 Godot 编辑器中逐一交互验证所有 Demo（滚动、触摸/拖动、动画、
  嵌套列表、滚动条行为等）。
- **过程可查** —— 仓库 git 历史按特性记录了开发过程：上述循环的每一步都以提交形式
  可见，AI 产出与人工修复均可追溯核对。

### 6. 提供给 AI 的参考资料

为保证与上游在 API 和行为上的忠实度，AI 全程参考：

- **AndroidX RecyclerView 全量源码**（AOSP, androidx.recyclerview, Apache-2.0）—— 作为
  移植的行为规格；
- **Godot 4 引擎源码**与 API 文档 —— 使代码贴合 Godot 惯例及 godot-cpp / GDExtension
  绑定层；
- 被移植 API 对应的 **Android 官方文档**。

### 7. 原创性与许可

本实现是在上述指导下依据 Android API 表面**原创重写**而成——并非复制 Android 源码。
资源以 **MIT** 许可发布；按 Apache License 的要求，上游 AOSP RecyclerView 的出处已在
`LICENSE.md` 中注明。本资源内所有 AI 生成的代码，均由作者（维护者）以相同 MIT 许可
贡献。
