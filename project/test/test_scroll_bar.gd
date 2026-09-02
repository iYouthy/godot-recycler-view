# Tests for the built-in ScrollBar integration: the RV owns an internal
# VScrollBar/HScrollBar (ScrollContainer-style), drives range/visibility by the
# scroll mode after every layout, mirrors reverse layouts, and auto-hides the
# bar via an alpha fade (Android-style).

extends GdUnitTestSuite


class BarAdapter extends Adapter:
	var count: int = 0
	var created := 0

	func _get_item_count() -> int:
		return count

	func _get_item_extent(_p: int) -> int:
		return 40

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


func _make_rv(count: int, horizontal := false) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(360, 600) if not horizontal else Vector2(600, 360))
	var adapter := BarAdapter.new()
	adapter.count = count
	rv.set_item_extent(40)
	rv.set_adapter(adapter)
	var layout := LinearLayoutManager.new()
	if horizontal:
		layout.set_orientation(LinearLayoutManager.HORIZONTAL)
	rv.set_layout(layout)
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_internal_bars_exist_and_active_axis_follows_layout() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	# Both bars exist as internal children of the RV.
	assert_that(rv.get_v_scroll_bar()).is_not_null()
	assert_that(rv.get_h_scroll_bar()).is_not_null()
	assert_that(rv.get_v_scroll_bar().get_parent()).is_same(rv)
	# A vertical layout drives the vertical bar; the horizontal one stays hidden.
	assert_that(rv.get_v_scroll_bar().is_visible()).is_true()
	assert_that(rv.get_h_scroll_bar().is_visible()).is_false()
	rv.free_items()
	rv.free()


func test_auto_mode_hides_bar_when_content_fits() -> void:
	# A short list (5x40 = 200px) fits the 600px viewport: no scrolling is
	# possible, so the bar must not show at all (Auto mode, default).
	var s := await _make_rv(5)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	assert_that(bar.is_visible()).is_false()
	# Growing the content past the viewport brings the bar up.
	s.adapter.count = 10000
	rv.notify_data_changed()
	await get_tree().process_frame
	assert_that(bar.is_visible()).is_true()
	# ...and shrinking it back hides the bar again.
	s.adapter.count = 5
	rv.notify_data_changed()
	await get_tree().process_frame
	assert_that(bar.is_visible()).is_false()
	rv.free_items()
	rv.free()


func test_bar_range_maps_offset_and_scrolls_rv() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	# Content 400000px, viewport 600px: max = content, page = viewport.
	assert_that(int(bar.get_max())).is_equal(400000)
	assert_that(int(bar.get_page())).is_equal(600)
	# RV scroll -> the bar's value follows the offset.
	rv.scroll_vertically(400)
	assert_that(int(bar.get_value())).is_equal(400)
	rv.scroll_vertically(-100)
	assert_that(int(bar.get_value())).is_equal(300)
	# Bar value_changed (user scroll) -> the RV scrolls to it.
	bar.set_value(2000)
	assert_that(rv.get_scroll_offset()).is_equal(2000)
	rv.free_items()
	rv.free()


func test_scroll_properties_alias_offsets() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	rv.set_v_scroll(500)
	assert_that(rv.get_v_scroll()).is_equal(500)
	assert_that(rv.get_scroll_offset()).is_equal(500)
	rv.free_items()
	rv.free()


func test_horizontal_layout_drives_h_bar_and_scrolls_rv() -> void:
	var s := await _make_rv(10000, true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_h_scroll_bar()
	assert_that(bar.is_visible()).is_true()
	assert_that(rv.get_v_scroll_bar().is_visible()).is_false()
	rv.scroll_horizontally(400)
	assert_that(int(bar.get_value())).is_equal(400)
	bar.set_value(1000)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(1000)
	rv.free_items()
	rv.free()


func test_never_show_hides_bar_but_keeps_scrolling() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_NEVER_SHOW)
	assert_that(rv.get_v_scroll_bar().is_visible()).is_false()
	# The RV itself still scrolls (wheel/drag/set_v_scroll).
	rv.set_v_scroll(800)
	assert_that(rv.get_scroll_offset()).is_equal(800)
	rv.free_items()
	rv.free()


func test_overlay_is_default_and_never_show_hides_bar() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	# Overlay is the default for both axes.
	assert_that(rv.get_vertical_scroll_mode()).is_equal(RecyclerView.SCROLL_MODE_OVERLAY)
	assert_that(rv.get_horizontal_scroll_mode()).is_equal(RecyclerView.SCROLL_MODE_OVERLAY)
	rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_NEVER_SHOW)
	assert_that(rv.get_v_scroll_bar().is_visible()).is_false()
	rv.free_items()
	rv.free()


func test_custom_step_forwards_to_bar() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	assert_that(rv.get_vertical_custom_step()).is_equal(-1.0)
	rv.set_vertical_custom_step(2.0)
	assert_that(rv.get_vertical_custom_step()).is_equal(2.0)
	assert_that(rv.get_v_scroll_bar().get_custom_step()).is_equal(2.0)
	rv.free_items()
	rv.free()


func test_reserve_mode_carves_viewport() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	var full_vp: Vector2 = rv.get_size()
	# Auto mode: the bar overlays the items, the viewport stays full size.
	assert_that(rv.get_viewport_size().x).is_equal(full_vp.x)
	# Reserve mode: the bar's thickness is carved out of the viewport.
	rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_RESERVE)
	var carved: Vector2 = rv.get_viewport_size()
	assert_that(carved.x).is_less(full_vp.x)
	assert_that(int(carved.x)).is_equal(int(full_vp.x - bar.get_minimum_size().x))
	rv.free_items()
	rv.free()


func test_inset_mode_carves_only_while_shown() -> void:
	# Inset pushes the content aside while the bar is shown (content overflows)
	# and grows it back to full width when the bar hides (content fits).
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	var full_w: float = rv.get_size().x
	rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_INSET)
	await get_tree().process_frame
	assert_that(bar.is_visible()).is_true()
	assert_that(rv.get_viewport_size().x).is_less(full_w)
	var item0 = rv.get_child_holder_at(0)
	if item0:
		assert_that(item0.get_control().size.x).is_less(full_w)
	# The content shrinks below one screen: the bar hides and the content
	# reclaims the full width (no leftover reserved strip).
	s.adapter.count = 5
	rv.notify_data_changed()
	await get_tree().process_frame
	await get_tree().process_frame
	assert_that(bar.is_visible()).is_false()
	assert_that(rv.get_viewport_size().x).is_equal(full_w)
	var item1 = rv.get_child_holder_at(0)
	if item1:
		assert_that(item1.get_control().size.x).is_equal(full_w)
	rv.free_items()
	rv.free()


func test_reserve_mode_reserves_space_even_when_hidden() -> void:
	# Reserve keeps the bar's space carved out even when the content fits and
	# the bar itself is hidden (no layout shift when content grows again).
	var s := await _make_rv(5)
	var rv: RecyclerView = s.rv
	rv.set_vertical_scroll_mode(RecyclerView.SCROLL_MODE_RESERVE)
	await get_tree().process_frame
	assert_that(rv.get_v_scroll_bar().is_visible()).is_false()
	assert_that(rv.get_viewport_size().x).is_less(rv.get_size().x)
	rv.free_items()
	rv.free()


func test_auto_hide_defaults_on_and_fades_after_idle() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	assert_that(rv.get_scroll_bar_auto_hide()).is_true()
	rv.set_scroll_bar_hide_delay(0.2)  # shorten the idle delay for the test
	# Scroll: the bar fades in and stays visible while recently active.
	rv.scroll_vertically(200)
	await get_tree().create_timer(0.1).timeout
	assert_that(bar.get_modulate().a).is_greater(0.5)
	# Sit idle past the hide delay: the bar fades out (and stops taking input).
	await get_tree().create_timer(0.6).timeout
	assert_that(bar.get_modulate().a).is_less(0.05)
	assert_that(bar.get_mouse_filter()).is_equal(Control.MOUSE_FILTER_IGNORE)
	rv.free_items()
	rv.free()


func test_auto_hide_off_keeps_bar_visible() -> void:
	var s := await _make_rv(10000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	rv.set_scroll_bar_auto_hide(false)
	rv.scroll_vertically(200)
	await get_tree().create_timer(0.8).timeout
	# Never fades out (past the 0.5s hide delay).
	assert_that(bar.get_modulate().a).is_greater(0.5)
	rv.free_items()
	rv.free()


func test_bar_stays_faded_when_content_fits() -> void:
	# A short list must not flash the bar on load: with nothing to scroll the
	# bar is hidden and its alpha stays 0 past the hide delay.
	var s := await _make_rv(5)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	await get_tree().create_timer(0.8).timeout
	assert_that(bar.get_modulate().a).is_less(0.05)
	rv.free_items()
	rv.free()


# The bars are the RV's last children, so Godot's hit-test finds them above
# the item views and routes their input natively. Clicking the track below the
# thumb pages forward (+page); above it pages back (-page).
func test_bar_flashes_when_content_grows_past_viewport() -> void:
	# Appending items until the list overflows must flash the bar once (the
	# auto-hide idle timer resets on appearance), then fade it out again.
	var s := await _make_rv(5)
	var rv: RecyclerView = s.rv
	var adapter: BarAdapter = s.adapter
	var bar = rv.get_v_scroll_bar()
	rv.set_scroll_bar_hide_delay(0.2)
	# Grow the content past the 600px viewport (5 -> 40 items).
	adapter.count = 40
	rv.notify_data_changed()
	await get_tree().create_timer(0.08).timeout
	# The bar appeared and is fading in (visible hint, not permanently hidden).
	assert_that(bar.is_visible()).is_true()
	assert_that(bar.get_modulate().a).is_greater(0.3)
	# Past the hide delay it fades out again.
	await get_tree().create_timer(0.6).timeout
	assert_that(bar.get_modulate().a).is_less(0.05)
	rv.free_items()
	rv.free()


func test_bar_track_click_pages_in_direction() -> void:
	var s := await _make_rv(1000)  # 40000px content, 600px viewport
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	rv.set_scroll_bar_auto_hide(false)
	rv.set_scroll_offset(10000)  # thumb near the top (25%)
	await get_tree().process_frame
	var bar_rect := bar.get_global_rect()
	# Click well below the thumb: pages forward.
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = bar_rect.position + Vector2(bar_rect.size.x * 0.5, bar_rect.size.y * 0.75)
	rv.get_viewport().push_input(press)
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = press.position
	rv.get_viewport().push_input(rel)
	assert_that(rv.get_scroll_offset()).is_equal(10600)
	# Click above the thumb: pages back.
	press.position = bar_rect.position + Vector2(bar_rect.size.x * 0.5, bar_rect.size.y * 0.1)
	rel.position = press.position
	rv.get_viewport().push_input(press)
	rv.get_viewport().push_input(rel)
	assert_that(rv.get_scroll_offset()).is_equal(10000)
	rv.free_items()
	rv.free()


# Dragging the thumb scrolls in the standard direction (thumb down = content
# up / scrolling forward, like ScrollContainer), keeps following the cursor
# once the drag leaves the bar's narrow strip, and ends on release.
func test_bar_thumb_drag_follows_mouse_outside_strip() -> void:
	var s := await _make_rv(1000)  # 40000px content -> a fat thumb (page ratio 600/40000)
	var rv: RecyclerView = s.rv
	var bar = rv.get_v_scroll_bar()
	rv.set_scroll_bar_auto_hide(false)
	rv.set_scroll_offset(20000)  # thumb centered on the track
	await get_tree().process_frame
	var bar_rect := bar.get_global_rect()
	var thumb_y := bar_rect.position.y + bar_rect.size.y * 0.5
	# Press on the thumb and drag DOWN 60px: the offset must grow by roughly the
	# 60px / 600px track share of the range (~4000), not a page step.
	var press := InputEventMouseButton.new()
	press.button_index = MOUSE_BUTTON_LEFT
	press.pressed = true
	press.position = bar_rect.position + Vector2(bar_rect.size.x * 0.5, thumb_y)
	rv.get_viewport().push_input(press)
	var mm := InputEventMouseMotion.new()
	mm.button_mask = MOUSE_BUTTON_MASK_LEFT
	mm.position = press.position + Vector2(0, 60)
	rv.get_viewport().push_input(mm)
	assert_that(rv.get_scroll_offset()).is_greater(23000)
	assert_that(rv.get_scroll_offset()).is_less(28000)
	# Drag further down but onto the RV's item area (off the bar strip): the
	# drag keeps following the cursor (events stay routed to the bar while the
	# button is held).
	var far := press.position + Vector2(-bar_rect.size.x * 2, 150)
	mm.position = far
	rv.get_viewport().push_input(mm)
	assert_that(rv.get_scroll_offset()).is_greater(30000)
	# Drag back up onto the item area above the bar: the offset comes back down.
	mm.position = far + Vector2(0, -220)
	rv.get_viewport().push_input(mm)
	assert_that(rv.get_scroll_offset()).is_less(30000)
	# Release ends the drag.
	var rel := InputEventMouseButton.new()
	rel.button_index = MOUSE_BUTTON_LEFT
	rel.pressed = false
	rel.position = mm.position
	rv.get_viewport().push_input(rel)
	var after_release := rv.get_scroll_offset()
	# A motion without the left button no longer scrolls the bar.
	mm.position = mm.position + Vector2(0, -50)
	rv.get_viewport().push_input(mm)
	assert_that(rv.get_scroll_offset()).is_equal(after_release)
	rv.free_items()
	rv.free()
