# Tests for the Adapter lifecycle callbacks (Android onViewRecycled /
# onFailedToRecycleView / onViewAttachedToWindow / onViewDetachedFromWindow):
# they are dispatched from the Recycler (cache overflow, scrap -> pool) and the
# RecyclerView (item added to / removed from the tree, recycle refused).

extends GdUnitTestSuite


class CallbackAdapter extends Adapter:
	var count: int = 0
	var created: int = 0
	var events: Array = []  # ordered [tag, position]

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		vh.set_control(Control.new())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass

	func _on_item_recycled(holder: ViewHolder) -> void:
		# get_position() must still be readable here (dispatch happens before
		# the holder's data is cleared, like Android).
		events.append(["recycled", holder.get_position()])

	func _on_view_attached(holder: ViewHolder) -> void:
		events.append(["attached", holder.get_position()])

	func _on_view_detached(holder: ViewHolder) -> void:
		events.append(["detached", holder.get_position()])


class StubbornAdapter extends CallbackAdapter:
	# Position 0 declares itself non-recyclable at bind time.
	var failed: int = 0
	var force: bool = false

	func _bind_item(holder: ViewHolder, position: int) -> void:
		if position == 0:
			holder.set_is_recyclable(false)

	func _on_failed_to_recycle_view(holder: ViewHolder) -> bool:
		failed += 1
		return force


func _make_setup(adapter := CallbackAdapter.new()) -> Dictionary:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	rv.set_item_extent(60)
	# Callback-count tests stay deterministic: prefetch would pre-create holders.
	rv.set_prefetch_enabled(false)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	return { "rv": rv, "adapter": adapter }


func _positions(events: Array, tag: String) -> Array:
	var out := []
	for e in events:
		if e[0] == tag:
			out.append(e[1])
	return out


func test_attach_and_detach_fire_on_scroll() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: CallbackAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	# The 10 visible items each attached once when entering the tree.
	assert_that(adapter.events.size()).is_equal(10)
	for e in adapter.events:
		assert_that(e[0]).is_equal("attached")

	adapter.events.clear()
	rv.scroll_vertically(60)
	# One item scrolled out (detached), one came in. The scrolled-out holder 0
	# fills the view cache, then the fill misses the cache for position 10 and
	# moves the cached holder into the pool (Android's recycleCachedViewAt),
	# dispatching on_item_recycled before it is bound to the new position.
	assert_that(adapter.events).is_equal([["detached", 0], ["recycled", 0], ["attached", 10]])
	rv.free_items()
	rv.free()


func test_cache_overflow_to_pool_dispatches_recycled() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: CallbackAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	adapter.events.clear()

	# Every scrolled-out holder ends up in the pool through the cache overflow
	# (the cache grows with the visible count, so no holder is discarded on a
	# single-item scroll; the fill moves it to the pool, dispatching
	# on_item_recycled with the old position still readable). Detach always
	# precedes the recycled dispatch, matching Android's order.
	rv.scroll_vertically(60)
	rv.scroll_vertically(60)
	rv.scroll_vertically(60)
	rv.scroll_vertically(60)
	assert_that(adapter.events).is_equal([
		["detached", 0], ["recycled", 0], ["attached", 10],
		["detached", 1], ["recycled", 1], ["attached", 11],
		["detached", 2], ["recycled", 2], ["attached", 12],
		["detached", 3], ["recycled", 3], ["attached", 13],
	])
	rv.free_items()
	rv.free()


func test_failed_to_recycle_default_keeps_holder_attached() -> void:
	var adapter := StubbornAdapter.new()
	adapter.force = false
	var setup := _make_setup(adapter)
	var rv: RecyclerView = setup.rv
	adapter.count = 100
	rv.request_layout()
	adapter.events.clear()

	rv.scroll_vertically(60)
	# The adapter was consulted and refused (default false): holder 0 stays in
	# the tree; only the new item attaches.
	assert_that(adapter.failed).is_equal(1)
	assert_that(adapter.events).is_equal([["attached", 10]])
	assert_that(rv.get_child_holder_count()).is_equal(11)
	var positions := []
	for i in rv.get_child_holder_count():
		positions.append(rv.get_child_holder_at(i).get_position())
	assert_that(positions).contains(0)
	rv.free_items()
	rv.free()


func test_failed_to_recycle_force_recycles() -> void:
	var adapter := StubbornAdapter.new()
	adapter.force = true
	var setup := _make_setup(adapter)
	var rv: RecyclerView = setup.rv
	adapter.count = 100
	rv.request_layout()
	adapter.events.clear()

	rv.scroll_vertically(60)
	# The adapter forced the recycle: holder 0 left the tree despite being
	# declared non-recyclable, then moved from the cache into the pool on the
	# fill miss (recycled dispatch) before being bound to position 10.
	assert_that(adapter.failed).is_equal(1)
	assert_that(adapter.events).is_equal([["detached", 0], ["recycled", 0], ["attached", 10]])
	assert_that(rv.get_child_holder_count()).is_equal(10)
	var positions := []
	for i in rv.get_child_holder_count():
		positions.append(rv.get_child_holder_at(i).get_position())
	assert_that(positions).not_contains(0)
	rv.free_items()
	rv.free()


func test_prefetch_does_not_dispatch_recycled() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: CallbackAdapter = setup.adapter
	adapter.count = 100
	rv.request_layout()
	adapter.events.clear()

	# Prefetched views are created unbound straight into the pool; they were
	# never bound to data, so no on_item_recycled (Android passes
	# dispatchRecycled=false for unbound views).
	rv.get_recycler().prefetch_view(10)
	assert_that(rv.get_recycler().get_recycled_view_count(0)).is_equal(1)
	assert_that(adapter.events).is_equal([])
	rv.free_items()
	rv.free()


func test_scrap_to_pool_dispatches_recycled() -> void:
	var setup := _make_setup()
	var rv: RecyclerView = setup.rv
	var adapter: CallbackAdapter = setup.adapter
	adapter.count = 10
	# No animator: the removed holder skips the fade-out and goes straight to
	# the changed scrap, which flushes into the pool at the end of the layout.
	rv.set_item_animator(null)
	rv.request_layout()
	adapter.events.clear()

	adapter.count = 9  # real usage: the adapter drops the item, then notifies
	rv.notify_item_range_removed(9, 1)
	await get_tree().process_frame  # notify defers the layout to end of frame
	# Removed the last visible item: it detached (left the tree), then the
	# scrap flush dispatched on_item_recycled as it entered the pool. Removed
	# holders report NO_POSITION (-1) here, like Android's
	# getBindingAdapterPosition() for removed holders.
	assert_that(adapter.events).is_equal([["detached", -1], ["recycled", -1]])
	# The flush reused the removed holder instead of fabricating a fresh view.
	assert_that(adapter.created).is_equal(10)
	rv.free_items()
	rv.free()
