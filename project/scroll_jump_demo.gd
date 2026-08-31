extends Control

# Scroll-jump demo: watch how the two scroll APIs affect view creation.
#
# scroll_to_position(pos)            -> instant jump (Android scrollToPosition):
#   one big layout swap. The view cache grows with the visible count and the
#   jump's fill reuses the cached holders by type, so created does not move.
# smooth_scroll_to_position(pos, d)  -> animated scroll (Android
#   smoothScrollToPosition): the offset advances frame by frame, each frame
#   recycling one row into the cache/pool and reusing it for the next — the
#   steady-state reuse path, also flat.
#
# What to look for: every operation reports its created delta. Both APIs must
# show +0 after the initial fill (any growth means a view was fabricated
# instead of reused — file a bug). The small-step button is the control:
# scrolling one row at a time reuses the pool and stays flat too.

const _EXTENT := 40
const _COUNT := 200

@onready var recycler_view: RecyclerView = %RecyclerView
@onready var info_label: Label = %InfoLabel
@onready var log_label: Label = %LogLabel

var _adapter: _NumberedAdapter
var _created_baseline := 0
var _last_created := 0
var _log: Array[String] = []


class _NumberedAdapter extends Adapter:
	var count := 200
	var created := 0

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		label.add_theme_font_size_override("font_size", 18)
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = "第 %d 行" % position


func _ready() -> void:
	_adapter = _NumberedAdapter.new()
	recycler_view.set_item_extent(_EXTENT)
	recycler_view.set_adapter(_adapter)
	recycler_view.set_layout(LinearLayoutManager.new())
	recycler_view.set_prefetch_enabled(true)
	recycler_view.request_layout()
	# First-scroll warm-up: the very first layout pass fills exactly the
	# viewport; any scroll then adds one boundary row (the fill extends one
	# item past the viewport edge), which fabricates a single view. Scroll 1px
	# out and back so every operation below shows a true +0 delta.
	recycler_view.scroll_vertically(1)
	recycler_view.scroll_vertically(-1)
	_last_created = _adapter.created
	_log.append("初始布局 + 预热：可见 %d 项，created %d" % [recycler_view.get_child_holder_count(), _adapter.created])

	%Jump100Button.pressed.connect(_on_jump_100)
	%Smooth100Button.pressed.connect(_on_smooth_100)
	%JumpTopButton.pressed.connect(_on_jump_top)
	%SmoothTopButton.pressed.connect(_on_smooth_top)
	%StepButton.pressed.connect(_on_step)


func _process(_delta: float) -> void:
	_update_info()


func _update_info() -> void:
	var pooled := recycler_view.get_recycler().get_recycled_view_count(0) \
			+ recycler_view.get_recycler().get_cached_view_count()
	var content := recycler_view.get_layout().get_content_size(recycler_view)
	info_label.text = (
		"created %d（操作增量：%+d）| 可见 %d | 缓存+池 %d\n"
		% [_adapter.created, _adapter.created - _last_created, recycler_view.get_child_holder_count(), pooled]
		+ "offset %d / %d（最大）| 列表 %d 项 × %dpx"
		% [recycler_view.get_scroll_offset(), content - int(recycler_view.get_viewport_size().y), _COUNT, _EXTENT]
	)
	var lines: Array[String] = ["── 操作记录（created 增量）──"]
	for l in _log.slice(maxi(_log.size() - 8, 0)):
		lines.append(l)
	log_label.text = "\n".join(lines)


func _record(op: String) -> void:
	var delta := _adapter.created - _last_created
	_log.append("%s：created %+d（%d → %d）" % [op, delta, _last_created, _adapter.created])
	_last_created = _adapter.created


func _on_jump_100() -> void:
	recycler_view.scroll_to_position(100)
	_record("瞬间跳 → 第 100 行")


func _on_smooth_100() -> void:
	recycler_view.smooth_scroll_to_position(100, 1.2)
	await _wait_settle()
	_record("平滑滚 → 第 100 行（完成后）")


func _on_jump_top() -> void:
	recycler_view.scroll_to_position(0)
	_record("瞬间跳 → 回顶")


func _on_smooth_top() -> void:
	recycler_view.smooth_scroll_to_position(0, 1.2)
	await _wait_settle()
	_record("平滑滚 → 回顶（完成后）")


func _on_step() -> void:
	recycler_view.scroll_vertically(_EXTENT)
	_record("小步滚动 +1 行")


func _wait_settle() -> void:
	# Smooth scrolls animate over frames; record the delta only once the
	# settle has finished so the log shows the true cost of the whole move.
	while recycler_view.get_scroll_state() == RecyclerView.SCROLL_STATE_SETTLING:
		await get_tree().process_frame
