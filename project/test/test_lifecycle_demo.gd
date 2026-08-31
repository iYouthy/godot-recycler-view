# Tests for lifecycle_demo's adapter: scrolling dispatches the four lifecycle
# callbacks (attach / detach / recycled / failed-to-recycle), recycled views
# are reused instead of recreated (created stays bounded), and the stubborn
# item's recycle is refused unless the adapter forces it.

extends GdUnitTestSuite

const LifecycleAdapterScript := preload("res://lifecycle_adapter.gd")


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 480))
	var adapter := LifecycleAdapterScript.new()
	rv.set_item_extent(40)
	rv.set_prefetch_enabled(false)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_scroll_dispatches_attach_detach_recycled() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: LifecycleAdapterScript = s.adapter
	# 480 / 40 = 12 visible; each attached once when entering the tree.
	assert_that(adapter.attached).is_equal(12)
	assert_that(adapter.created).is_equal(12)
	assert_that(adapter.recycled).is_equal(0)

	# Scroll five items: five detached (left the tree), five attached. The first
	# recycled holder fills the single view-cache slot (no dispatch); the next
	# four overflow into the pool (on_item_recycled with the old position). The
	# pooled views are reused for the incoming items (Android's pool matches by
	# view type only, not position), so only one fresh view is created.
	rv.scroll_vertically(200)
	assert_that(adapter.detached).is_equal(5)
	assert_that(adapter.attached).is_equal(17)
	assert_that(adapter.recycled).is_equal(4)
	assert_that(adapter.created).is_equal(13)
	rv.free_items()
	rv.free()


func test_scroll_back_reuses_pooled_views() -> void:
	# Scrolling back must rebind the pooled views instead of creating new ones:
	# the created counter stops growing (the virtualized list is cheap exactly
	# because of this reuse).
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: LifecycleAdapterScript = s.adapter
	rv.scroll_vertically(200)
	assert_that(adapter.created).is_equal(13)

	rv.scroll_vertically(-200)
	# Scrolling back rebinds the pooled views: the created counter does not
	# move at all (the virtualized list is cheap exactly because of this).
	assert_that(adapter.created).is_equal(13)
	assert_that(adapter.attached).is_equal(22)
	assert_that(adapter.detached).is_equal(10)
	# Five more holders overflowed the cache into the pool (5 victims + the one
	# cached from the forward scroll were dispatched: 4 + 5 = 9).
	assert_that(adapter.recycled).is_equal(9)
	rv.free_items()
	rv.free()


func test_stubborn_item_refuses_recycle_unless_forced() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: LifecycleAdapterScript = s.adapter
	# Jump to the stubborn item (position 40), then one more item so it leaves
	# the viewport: the layout consults _on_failed_to_recycle_view. With
	# force=false it refuses and the holder stays attached — a ghost pinned to
	# its slot.
	rv.scroll_vertically(1600)
	rv.scroll_vertically(40)
	assert_that(adapter.failed).is_equal(1)
	# 12 visible items + the stubborn ghost.
	assert_that(rv.get_child_holder_count()).is_equal(13)
	var positions := []
	for i in rv.get_child_holder_count():
		positions.append(rv.get_child_holder_at(i).get_position())
	assert_that(positions).contains(LifecycleAdapterScript.STUBBORN_POS)

	# Forced: the next consultation recycles it anyway.
	adapter.force = true
	rv.scroll_vertically(40)
	assert_that(adapter.failed).is_equal(2)
	assert_that(rv.get_child_holder_count()).is_equal(12)
	var positions_after := []
	for i in rv.get_child_holder_count():
		positions_after.append(rv.get_child_holder_at(i).get_position())
	assert_that(positions_after).not_contains(LifecycleAdapterScript.STUBBORN_POS)
	rv.free_items()
	rv.free()
