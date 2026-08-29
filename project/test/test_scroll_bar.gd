# Tests for the ScrollBar protocol (base Control) and DefaultScrollBar: attach
# via RecyclerView.set_scroll_bar, scroll notification + data contract, thumb
# geometry, custom subclassing, and auto-hide.

extends GdUnitTestSuite


class BarAdapter extends Adapter:
	var count: int = 0
	var created := 0

	func _get_item_count() -> int:
		return count

	func _get_item_height(_p: int) -> int:
		return 40

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh


class CountingBar extends RecyclerViewScrollBar:
	var calls := 0

	func _on_scroll_changed() -> void:
		calls += 1


func _make_rv(with_bar: bool) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.set_size(Vector2(360, 600))
	var adapter := BarAdapter.new()
	adapter.count = 10000
	rv.set_item_size(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	if with_bar:
		rv.set_scroll_bar(DefaultScrollBar.new())
	get_tree().root.add_child(rv)
	rv.request_layout()
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_set_scroll_bar_attaches_as_child() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	assert_that(bar).is_not_null()
	assert_that(bar.get_parent()).is_same(rv)
	# A vertical RV picks the vertical axis for its bar.
	assert_that(bar.get_axis()).is_equal(RecyclerViewScrollBar.SCROLL_BAR_VERTICAL)
	rv.free_items()
	rv.free()


func test_scroll_notifies_and_offset_follows() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	rv.scroll_vertically(400)
	assert_that(bar.get_offset()).is_equal(400)
	rv.scroll_vertically(-100)
	assert_that(bar.get_offset()).is_equal(300)
	rv.free_items()
	rv.free()


func test_thumb_geometry_follows_offset() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	# Content 400000px, viewport 600px: a tiny thumb pinned at the top.
	var top: Rect2 = bar.get_thumb_rect()
	assert_that(int(top.position.y)).is_equal(0)
	assert_that(top.size.y).is_greater(0)
	# Scroll to the middle of the content: the thumb moves down.
	rv.scroll_vertically(200000)
	var mid: Rect2 = bar.get_thumb_rect()
	assert_that(mid.position.y).is_greater(0)
	assert_that(mid.position.y).is_greater(top.position.y)
	rv.free_items()
	rv.free()


func test_custom_scroll_bar_receives_notifications() -> void:
	var s := await _make_rv(false)
	var rv: RecyclerView = s.rv
	var bar := CountingBar.new()
	rv.set_scroll_bar(bar)
	var calls_after_attach := bar.calls
	# Scroll and layout both notify the bar.
	rv.scroll_vertically(200)
	assert_that(bar.calls).is_greater(calls_after_attach)
	rv.free_items()
	rv.free()


func test_auto_hide_default_on_and_hides_after_idle() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	# Default is on, forwarded to the bar.
	assert_that(rv.get_scroll_bar_auto_hide()).is_true()
	assert_that(bar.get_auto_hide()).is_true()
	bar.set_hide_delay(0.2)  # shorten the idle delay for the test
	# Scroll: the bar fades in.
	rv.scroll_vertically(200)
	await get_tree().create_timer(0.1).timeout
	assert_that(bar.get_modulate().a).is_greater(0.5)
	# Sit idle past the hide delay: the bar fades out.
	await get_tree().create_timer(0.5).timeout
	assert_that(bar.get_modulate().a).is_less(0.5)
	rv.free_items()
	rv.free()


func test_auto_hide_off_keeps_bar_visible() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	rv.set_scroll_bar_auto_hide(false)
	assert_that(bar.get_auto_hide()).is_false()
	rv.scroll_vertically(200)
	await get_tree().create_timer(0.8).timeout
	# Never fades out.
	assert_that(bar.get_modulate().a).is_greater(0.5)
	rv.free_items()
	rv.free()


func test_hide_delay_defaults_to_android_and_forwards() -> void:
	var s := await _make_rv(true)
	var rv: RecyclerView = s.rv
	var bar = rv.get_scroll_bar()
	# Android's default scrollbar fade delay is 500ms.
	assert_that(rv.get_scroll_bar_hide_delay()).is_equal(0.5)
	assert_that(bar.get_hide_delay()).is_equal(0.5)
	# The RV inspector setting forwards to the bar.
	rv.set_scroll_bar_hide_delay(1.0)
	assert_that(rv.get_scroll_bar_hide_delay()).is_equal(1.0)
	assert_that(bar.get_hide_delay()).is_equal(1.0)
	rv.free_items()
	rv.free()
