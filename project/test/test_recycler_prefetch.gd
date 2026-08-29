# Tests for the Recycler prefetch and the reuse guarantee: scrolling through a
# long list must not fabricate a fresh holder on every layout pass. Scrolled-out
# holders flow back through the cache/pool and incoming items reuse them;
# prefetch only pre-creates into the pool when every cache is empty (it does not
# top the pool back up on each pass, which would make created grow forever).

extends GdUnitTestSuite


class PrefetchAdapter extends Adapter:
	var item_count := 100
	var created := 0

	func _get_item_count() -> int:
		return item_count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(200, 40))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(position)


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 200))  # 5 visible items at 40px
	var adapter := PrefetchAdapter.new()
	rv.set_item_size(40)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func test_no_prefetch_without_scroll() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	# Nothing scrolled yet, so no runway was prefetched into the pool.
	assert_that(rv.get_recycler().get_recycled_view_count(0)).is_equal(0)
	rv.free_items()
	rv.free()


func test_scrolling_reuses_holders_created_stays_bounded() -> void:
	# Regression: scrolling through a long list must reuse scrolled-out holders
	# instead of creating a fresh view every pass. After the first window the
	# created counter stays constant no matter how far we scroll.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: PrefetchAdapter = s.adapter
	rv.scroll_vertically(400)  # jump far past the first window
	var created_after_jump := adapter.created
	for i in 40:
		rv.scroll_vertically(40)  # one item per pass, down to the end
	assert_that(adapter.created).is_equal(created_after_jump)
	rv.free_items()
	rv.free()


func test_scroll_up_also_reuses_not_creates() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: PrefetchAdapter = s.adapter
	rv.scroll_vertically(400)
	rv.scroll_vertically(-200)
	var created_after := adapter.created
	for i in 40:
		rv.scroll_vertically(-40)  # scroll back up, reusing all the way
	assert_that(adapter.created).is_equal(created_after)
	rv.free_items()
	rv.free()


func test_prefetched_holder_reused_when_scrolled_to() -> void:
	# Prefetch position 5 manually (auto prefetch off), then scroll it into
	# view: position 5 reuses the prefetched holder instead of creating one, so
	# one fewer holder is created than without prefetch.
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: PrefetchAdapter = s.adapter
	rv.set_prefetch_enabled(false)
	rv.get_recycler().prefetch_view(5)
	var created_after_prefetch := adapter.created  # 5 visible + 1 prefetched = 6

	# Scroll so position 5 enters the viewport (visible becomes 1..5).
	rv.scroll_vertically(40)
	# Positions 1..4 were already visible; only 5 is new, and it came from the
	# pool instead of being created (without prefetch this would create one
	# more holder).
	assert_that(adapter.created).is_equal(created_after_prefetch)
	# The prefetched holder is actually showing position 5.
	var found := false
	for i in rv.get_child_holder_count():
		if rv.get_child_holder_at(i).get_position() == 5:
			found = true
	assert_that(found).is_true()
	rv.free_items()
	rv.free()
