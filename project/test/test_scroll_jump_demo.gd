# Tests for scroll_jump_demo: both scroll APIs must reach their target without
# fabricating views. scroll_to_position does one big layout swap that reuses
# the viewport-sized cache; smooth_scroll_to_position animates frame by frame
# through the steady-state reuse path. Either one growing `created` means a
# holder was built instead of reused.

extends GdUnitTestSuite

const _DEMO := preload("res://scroll_jump_demo.gd")


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 480))
	rv.set_item_extent(40)
	rv.set_prefetch_enabled(false)
	var adapter := _DEMO._NumberedAdapter.new()
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	# Same warm-up as the demo: the first scroll fabricates the one boundary
	# row past the viewport edge; after it, every operation must be +0.
	rv.scroll_vertically(1)
	rv.scroll_vertically(-1)
	return { "rv": rv, "adapter": adapter }


func test_instant_jump_does_not_create() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: _DEMO._NumberedAdapter = s.adapter
	var created_before := adapter.created
	rv.scroll_to_position(100)
	assert_that(rv.get_scroll_offset()).is_equal(100 * 40)
	assert_that(adapter.created).is_equal(created_before)
	# And back.
	rv.scroll_to_position(0)
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_smooth_scroll_does_not_create() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: _DEMO._NumberedAdapter = s.adapter
	var created_before := adapter.created
	rv.smooth_scroll_to_position(100, 0.6)
	# Wait for the settle animation to finish.
	while rv.get_scroll_state() == RecyclerView.SCROLL_STATE_SETTLING:
		await get_tree().process_frame
	assert_that(rv.get_scroll_offset()).is_equal(100 * 40)
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()


func test_small_steps_stay_flat() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: _DEMO._NumberedAdapter = s.adapter
	var created_before := adapter.created
	for i in 10:
		rv.scroll_vertically(40)
	assert_that(adapter.created).is_equal(created_before)
	rv.free_items()
	rv.free()
