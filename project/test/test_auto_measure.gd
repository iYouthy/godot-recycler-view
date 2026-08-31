# Tests for RecyclerView.auto_measure_items: content-driven item sizes.
#
# When enabled, each item's extent along the scroll axis is measured from the
# item control itself instead of the static item extent (Android's
# wrap_content / match_parent):
#   - the root control's combined minimum size decides the slot, clamped by
#     its combined maximum size when one is declared;
#   - a root control with SIZE_EXPAND along the scroll axis fills the viewport
#     (match_parent);
#   - measured extents are cached by position, cleared on any data change and
#     re-measured by the next layout;
#   - scroll_to_position / smooth_scroll_to_position re-anchor to the exact
#     target once the measured extents settle.
# While disabled, the static extent path is unchanged (test_off_by_default).

extends GdUnitTestSuite

const _LONG_TEXT := "这是一个足够长的段落,用来让 RichTextLabel 在设置好宽度之后换行成多行,内容高度因此明显大于单行文本的高度,以此验证测量机制真的读取了内容而不是静态的 item_extent。"
const _SHORT_TEXT := "短文本"


class _RichTextAdapter extends Adapter:
	var texts: Array[String] = []
	var created := 0

	func _init(p_texts: Array[String]) -> void:
		texts = p_texts

	func _get_item_count() -> int:
		return texts.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var rtl := RichTextLabel.new()
		rtl.set_fit_content(true)
		rtl.set_autowrap_mode(TextServer.AUTOWRAP_WORD_SMART)
		rtl.add_theme_font_size_override("normal_font_size", 20)
		vh.set_control(rtl)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as RichTextLabel).set_text(texts[position])


# An item root that is a container (header + RichTextLabel): the measured
# extent must be the container's own minimum (children sum), with the
# RichTextLabel shaped at the final width — not inflated by its pre-layout
# 0-width shaping.
class _BoxedTextAdapter extends Adapter:
	var texts: Array[String] = []
	var created := 0

	func _init(p_texts: Array[String]) -> void:
		texts = p_texts

	func _get_item_count() -> int:
		return texts.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var box := VBoxContainer.new()
		var header := Label.new()
		header.add_theme_font_size_override("font_size", 12)
		box.add_child(header)
		var rtl := RichTextLabel.new()
		rtl.set_fit_content(true)
		rtl.set_autowrap_mode(TextServer.AUTOWRAP_WORD_SMART)
		rtl.add_theme_font_size_override("normal_font_size", 20)
		box.add_child(rtl)
		vh.set_control(box)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var box := holder.get_control() as VBoxContainer
		(box.get_child(0) as Label).text = "消息 #%d" % position
		(box.get_child(1) as RichTextLabel).set_text(texts[position])


# A root with SIZE_EXPAND along the scroll axis: must fill the viewport.
class _ExpandAdapter extends Adapter:
	var count := 3

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		var box := PanelContainer.new()
		box.size_flags_vertical = Control.SIZE_EXPAND
		vh.set_control(box)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


func _make_setup(texts: Array[String], auto_measure := true) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(300, 480))
	rv.set_item_extent(64)
	rv.set_auto_measure_items(auto_measure)
	rv.set_prefetch_enabled(false)
	var adapter := _RichTextAdapter.new(texts)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_measure_reflects_content() -> void:
	var s := await _make_setup([_LONG_TEXT, _SHORT_TEXT])
	var rv: RecyclerView = s.rv
	# The long text wraps to several lines at the 300px width and must measure
	# taller than both the default extent and the short text.
	assert_that(rv.get_item_extent(0)).is_greater(64)
	assert_that(rv.get_item_extent(1)).is_less(rv.get_item_extent(0))
	assert_that(rv.get_item_extent(1)).is_greater(0)
	rv.free_items()
	rv.free()


func test_rows_do_not_overlap() -> void:
	var texts: Array[String] = [_LONG_TEXT, _SHORT_TEXT, _LONG_TEXT, _SHORT_TEXT]
	var s := await _make_setup(texts)
	var rv: RecyclerView = s.rv
	var layout := rv.get_layout() as LinearLayoutManager
	# The offset table is the accumulated measured extents: consecutive item
	# tops are exactly extent apart, so slots never overlap and never gap.
	for i in texts.size() - 1:
		assert_that(layout.get_item_offset(i + 1)).is_equal(layout.get_item_offset(i) + rv.get_item_extent(i))
	# And the laid-out controls sit in those slots.
	var top := 0
	for i in rv.get_child_holder_count():
		var holder := rv.get_child_holder_at(i)
		var control := holder.get_control()
		assert_that(int(control.position.y)).is_equal(layout.get_item_offset(holder.get_position()))
		assert_that(int(control.size.y)).is_equal(rv.get_item_extent(holder.get_position()))
	rv.free_items()
	rv.free()


func test_content_size_is_measured_sum() -> void:
	var s := await _make_setup([_LONG_TEXT, _SHORT_TEXT, _LONG_TEXT])
	var rv: RecyclerView = s.rv
	var sum := 0
	for i in 3:
		sum += rv.get_item_extent(i)
	assert_that(rv.get_layout().get_content_size(rv)).is_equal(sum)
	rv.free_items()
	rv.free()


func test_off_by_default() -> void:
	# The static path must be bit-identical while the feature is off.
	var s := await _make_setup([_LONG_TEXT, _SHORT_TEXT], false)
	var rv: RecyclerView = s.rv
	assert_that(rv.get_item_extent(0)).is_equal(64)
	assert_that(rv.get_item_extent(1)).is_equal(64)
	rv.free_items()
	rv.free()


func test_update_remeasures_changed_rows() -> void:
	var s := await _make_setup([_SHORT_TEXT, _SHORT_TEXT, _SHORT_TEXT])
	var rv: RecyclerView = s.rv
	var adapter: _RichTextAdapter = s.adapter
	var before := rv.get_item_extent(0)
	adapter.texts[1] = _LONG_TEXT
	adapter.notify_item_changed(1)
	await get_tree().process_frame
	# The row's text grew: its measured extent must follow, and the rows below
	# shift down by exactly the growth (no overlap after the update).
	var grew := rv.get_item_extent(1) - before
	assert_that(grew).is_greater(0)
	assert_that(rv.get_layout().get_item_offset(2)).is_equal(rv.get_item_extent(0) + rv.get_item_extent(1))
	rv.free_items()
	rv.free()


func test_expand_fills_viewport() -> void:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(300, 480))
	rv.set_item_extent(64)
	rv.set_auto_measure_items(true)
	var adapter := _ExpandAdapter.new()
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	# SIZE_EXPAND along the scroll axis = match_parent: the slot is the viewport.
	assert_that(rv.get_item_extent(0)).is_equal(480)
	assert_that(rv.get_item_extent(1)).is_equal(480)
	assert_that(rv.get_child_holder_at(0).get_control().size.y).is_equal(480.0)
	rv.free_items()
	rv.free()


func test_measure_container_root_shapes_children_at_width() -> void:
	# Regression: a RichTextLabel inside a VBox must be shaped at the final
	# width before the box's minimum size is read. Without the preset pass it
	# shapes at its initial 0 width and reports one line per character,
	# inflating the extent to thousands of pixels.
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(300, 480))
	rv.set_item_extent(64)
	rv.set_auto_measure_items(true)
	rv.set_prefetch_enabled(false)
	var adapter := _BoxedTextAdapter.new([_SHORT_TEXT, _LONG_TEXT, _SHORT_TEXT])
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	# The box is header + shaped RichTextLabel: a sane small height, and the
	# long text's box clearly taller than the short text's.
	assert_that(rv.get_item_extent(0)).is_greater(30)
	assert_that(rv.get_item_extent(0)).is_less(200)
	assert_that(rv.get_item_extent(1)).is_greater(rv.get_item_extent(0))
	assert_that(rv.get_item_extent(1)).is_less(400)
	# Rows below the long text start exactly where its measured height ends.
	assert_that(rv.get_layout().get_item_offset(2)).is_equal(rv.get_item_extent(0) + rv.get_item_extent(1))
	rv.free_items()
	rv.free()


func test_expand_follows_viewport_resize() -> void:
	# Regression: SIZE_EXPAND extents are viewport-derived; a resize must drop
	# the stale measurement or the banner keeps the old (larger) height and
	# overflows the RecyclerView.
	var s := await _make_setup([_SHORT_TEXT])
	var rv: RecyclerView = s.rv
	rv.set_auto_measure_items(true)
	var expand_adapter := _ExpandAdapter.new()
	rv.set_adapter(expand_adapter)
	await get_tree().process_frame
	assert_that(rv.get_item_extent(0)).is_equal(480)
	rv.set_size(Vector2(300, 240))
	await get_tree().process_frame
	assert_that(rv.get_item_extent(0)).is_equal(240)
	assert_that(rv.get_child_holder_at(0).get_control().size.y).is_equal(240.0)
	rv.free_items()
	rv.free()


func test_measure_respects_maximum_size() -> void:
	# A declared maximum size caps the measured extent (custom_maximum_size;
	# only honored on engines with the combined maximum-size API, 4.5+). The
	# width must stay > 0: RichTextLabel treats a 0 maximum width as "no wrap
	# width limit", which would disable wrapping and collapse the content.
	var s := await _make_setup([_LONG_TEXT])
	var rv: RecyclerView = s.rv
	var holder := rv.get_child_holder_at(0)
	var control := holder.get_control() as RichTextLabel
	if not control.has_method("set_custom_maximum_size"):
		rv.free_items()
		rv.free()
		return
	control.custom_maximum_size = Vector2(300, 80)
	# Force a re-measure: clear the cache through a data change.
	(s.adapter as _RichTextAdapter).notify_item_changed(0)
	await get_tree().process_frame
	assert_that(rv.get_item_extent(0)).is_equal(80)
	rv.free_items()
	rv.free()


func test_scroll_to_position_reanchors() -> void:
	# The target's region starts out unmeasured (estimated at 64px); the jump
	# lands on the estimate, the layout measures, and the pending target
	# re-anchors the offset to the exact measured position.
	var texts: Array[String] = []
	for i in 200:
		texts.append(_LONG_TEXT if i % 3 == 0 else _SHORT_TEXT)
	var s := await _make_setup(texts)
	var rv: RecyclerView = s.rv
	rv.scroll_to_position(100)
	assert_that(rv.get_scroll_offset()).is_equal(rv.get_layout().get_item_offset(100))
	# And scrolling back re-anchors too.
	rv.scroll_to_position(0)
	assert_that(rv.get_scroll_offset()).is_equal(0)
	rv.free_items()
	rv.free()


func test_smooth_scroll_reanchors_after_settle() -> void:
	var texts: Array[String] = []
	for i in 200:
		texts.append(_LONG_TEXT if i % 3 == 0 else _SHORT_TEXT)
	var s := await _make_setup(texts)
	var rv: RecyclerView = s.rv
	rv.smooth_scroll_to_position(100, 0.3)
	while rv.get_scroll_state() == RecyclerView.SCROLL_STATE_SETTLING:
		await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(rv.get_layout().get_item_offset(100))
	rv.free_items()
	rv.free()


func test_append_after_scrolling_to_end_keeps_items() -> void:
	# Regression: after a jump to the end, appending a row used to clear the
	# measured cache, shrink the estimated content below the current offset and
	# leave the list looking empty (the offset was never clamped because no
	# set_scroll_offset call happened). The layout must clamp and keep the rows.
	var texts: Array[String] = []
	for i in 20:
		texts.append(_LONG_TEXT if i % 2 == 0 else _SHORT_TEXT)
	var s := await _make_setup(texts)
	var rv: RecyclerView = s.rv
	var adapter: _RichTextAdapter = s.adapter
	rv.scroll_to_position(texts.size() - 1)
	await get_tree().process_frame
	assert_that(rv.get_child_holder_count()).is_greater(0)
	adapter.texts.append(_SHORT_TEXT)
	adapter.notify_item_inserted(adapter.texts.size() - 1)
	await get_tree().process_frame
	# The list must still show rows...
	assert_that(rv.get_child_holder_count()).is_greater(0)
	# ...and stay scrollable (the offset is clamped inside the layout, no
	# pending re-anchor may yank it back after a drag).
	var before := rv.get_scroll_offset()
	rv.scroll_vertically(-40)
	assert_that(rv.get_scroll_offset()).is_less(before)
	rv.free_items()
	rv.free()


func test_height_resize_keeps_wrap_measurements() -> void:
	# Regression: a pure height change must not drop wrap_content
	# measurements (their height depends on the width only) — otherwise the
	# list re-measures everything while scrolling after a resize and jitters.
	var s := await _make_setup([_LONG_TEXT, _SHORT_TEXT, _LONG_TEXT])
	var rv: RecyclerView = s.rv
	var before := [rv.get_item_extent(0), rv.get_item_extent(1), rv.get_item_extent(2)]
	rv.set_size(Vector2(300, 600))
	await get_tree().process_frame
	assert_that(rv.get_item_extent(0)).is_equal(before[0])
	assert_that(rv.get_item_extent(1)).is_equal(before[1])
	assert_that(rv.get_item_extent(2)).is_equal(before[2])
	rv.free_items()
	rv.free()


func test_width_resize_remeasures() -> void:
	# A width change re-wraps every row: the measurements must be dropped and
	# the long text must measure taller at the narrower width.
	var s := await _make_setup([_LONG_TEXT])
	var rv: RecyclerView = s.rv
	var before := rv.get_item_extent(0)
	rv.set_size(Vector2(150, 480))
	await get_tree().process_frame
	assert_that(rv.get_item_extent(0)).is_greater(before)
	rv.free_items()
	rv.free()


# A match_parent banner followed by wrap_content rows: used to reproduce the
# stale-offset-table overlap after consecutive resizes.
class _BannerTextAdapter extends Adapter:
	var texts: Array[String] = []
	var created := 0

	func _init(p_texts: Array[String]) -> void:
		texts = p_texts

	func _get_item_count() -> int:
		return texts.size()

	func _get_item_view_type(position: int) -> int:
		return 1 if position == 0 else 0

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		if view_type == 1:
			var banner := PanelContainer.new()
			banner.size_flags_vertical = Control.SIZE_EXPAND
			vh.set_control(banner)
		else:
			var rtl := RichTextLabel.new()
			rtl.set_fit_content(true)
			rtl.set_autowrap_mode(TextServer.AUTOWRAP_WORD_SMART)
			rtl.add_theme_font_size_override("normal_font_size", 20)
			vh.set_control(rtl)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		if position == 0:
			return
		(holder.get_control() as RichTextLabel).text = texts[position]


func _banner_overlaps(rv: RecyclerView) -> bool:
	for i in rv.get_child_holder_count():
		var a := rv.get_child_holder_at(i)
		if a.get_position() != 0:
			continue
		var ca := a.get_control()
		for j in rv.get_child_holder_count():
			var b := rv.get_child_holder_at(j)
			if b.get_position() == 0:
				continue
			var cb := b.get_control()
			if ca.position.y < cb.position.y + cb.size.y and cb.position.y < ca.position.y + ca.size.y:
				return true
	return false


func test_consecutive_resize_does_not_overlap() -> void:
	# Regression: resizing the viewport every frame (dragging the window edge)
	# left the offset table stale — the dropped match_parent measurement only
	# invalidated the table when a measurement differed, but the previous
	# frame's layout had left the table clean, so the row below the taller
	# banner kept its old slot and overlapped it.
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(300, 480))
	rv.set_item_extent(64)
	rv.set_auto_measure_items(true)
	rv.set_prefetch_enabled(true)
	var texts: Array[String] = [_SHORT_TEXT]
	for i in 20:
		texts.append(_LONG_TEXT if i % 2 == 0 else _SHORT_TEXT)
	var adapter := _BannerTextAdapter.new(texts)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	# Scroll away and back to the top, then grow the viewport every frame.
	rv.scroll_vertically(300)
	rv.scroll_vertically(-300)
	assert_that(rv.get_scroll_offset()).is_equal(0)
	for i in 21:
		rv.set_size(Vector2(300, rv.get_size().y + 20))
		await get_tree().process_frame
	# The banner re-measured to the new viewport and the table must agree:
	# the first wrap_content row starts exactly where the banner ends.
	assert_that(_banner_overlaps(rv)).is_false()
	assert_that(rv.get_item_extent(0)).is_equal(900)
	rv.free_items()
	rv.free()


func test_jump_to_end_then_drag_is_not_yanked() -> void:
	# Regression: jumping to the last row (shorter than the viewport, so its
	# start lies beyond the maximum scroll offset) used to leave the pending
	# re-anchor unsettled; every later layout then re-applied the clamped end
	# offset, swallowing user drags. The clamped landing spot must count as
	# reached.
	var texts: Array[String] = []
	for i in 20:
		texts.append(_LONG_TEXT if i % 2 == 0 else _SHORT_TEXT)
	var s := await _make_setup(texts)
	var rv: RecyclerView = s.rv
	rv.scroll_to_position(texts.size() - 1)
	await get_tree().process_frame
	rv.scroll_vertically(-40)
	var after_drag := rv.get_scroll_offset()
	assert_that(after_drag).is_less(rv.get_layout().get_content_size(rv) - int(rv.get_viewport_size().y))
	rv.scroll_vertically(-40)
	assert_that(rv.get_scroll_offset()).is_less(after_drag)
	rv.free_items()
	rv.free()
