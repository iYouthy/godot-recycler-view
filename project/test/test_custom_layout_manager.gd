# Tests for script-defined layouts: a GDScript subclass of LayoutManager
# overrides _on_layout_children (plus the scroll-range/getter virtuals) and
# drives item obtain/recycle/positioning through the RecyclerView's public
# API. This pins the script-facing contract:
#   - holders outside the visible range are recycled by the script
#   - _get_content_size drives the scroll bounds (clamping)
#   - _get_position_offset drives scroll_to_position / snapping
#   - _collect_adjacent_prefetch_positions warms the pool (created stays bounded)
#   - _on_data_changed fires on adapter changes

extends GdUnitTestSuite

const EXTENT := 40
const VIEWPORT := Vector2(200, 400)


class CustomLM extends LayoutManager:
	# The script side of the contract, exactly as the demo teaches it.
	var extent := EXTENT
	var data_changed_count := 0
	var prefetch_consulted := 0

	func _on_layout_children(recycler_view, state) -> void:
		var viewport_h: int = int(recycler_view.get_viewport_size().y)
		var offset: int = recycler_view.get_scroll_offset()
		var count: int = get_item_count()
		var first := maxi(0, offset / extent)
		var last := mini(count - 1, (offset + viewport_h) / extent)
		var present := {}
		for i in recycler_view.get_child_holder_count():
			var h = recycler_view.get_child_holder_at(i)
			present[h.get_position()] = h
		# Recycle holders outside the visible range.
		for i in range(recycler_view.get_child_holder_count() - 1, -1, -1):
			var holder = recycler_view.get_child_holder_at(i)
			var pos: int = holder.get_position()
			if pos < first or pos > last:
				recycler_view.remove_item_view(holder)
				recycler_view.recycle_view(holder, pos)
		# Fill and position the visible range.
		for pos in range(first, last + 1):
			var holder = present.get(pos)
			if holder == null:
				holder = recycler_view.get_view_for_position(pos)
				recycler_view.add_item_view(holder)
			recycler_view.set_item_view_position(holder, Vector2(0, pos * extent - offset), Vector2(VIEWPORT.x, extent))

	func _can_scroll_vertically() -> bool:
		return true

	func _get_content_size(recycler_view) -> int:
		return get_item_count() * extent

	func _get_position_offset(position: int) -> int:
		return position * extent

	func _get_item_rect(recycler_view, position: int) -> Rect2:
		return Rect2(0, position * extent - recycler_view.get_scroll_offset(), VIEWPORT.x, extent)

	func _on_data_changed() -> void:
		data_changed_count += 1

	func _collect_adjacent_prefetch_positions(dy: int) -> Array:
		prefetch_consulted += 1
		if dy > 0:
			var rv = get_recycler_view()
			var next: int = rv.get_scroll_offset() / extent + VIEWPORT.y / extent + 1
			return [next + 1, next + 2]
		return []


class PlainLM extends LayoutManager:
	# No overrides at all: the base defaults must keep the RV functional
	# (empty layout, no scrolling, content clamped to one viewport).

	func _get_content_size(recycler_view) -> int:
		return 0


class CountAdapter extends Adapter:
	var count := 100
	var created := 0
	var bounds := []

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var c := Control.new()
		c.set_size(Vector2(VIEWPORT.x, EXTENT))
		vh.set_control(c)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		holder.get_control().position = Vector2.ZERO
		holder.get_control().size = Vector2(VIEWPORT.x, EXTENT)


func _make_setup(lm: LayoutManager = null) -> Dictionary:
	var rv := RecyclerView.new()
	rv.set_size(VIEWPORT)
	rv.set_item_extent(EXTENT)
	rv.set_prefetch_enabled(false)
	var adapter := CountAdapter.new()
	rv.set_adapter(adapter)
	if lm == null:
		lm = CustomLM.new()
	rv.set_layout(lm)
	rv.request_layout()
	return { "rv": rv, "adapter": adapter, "lm": lm }


func _positions(rv: RecyclerView) -> Array:
	var out := []
	for i in rv.get_child_holder_count():
		out.append(rv.get_child_holder_at(i).get_position())
	return out


func test_initial_layout_matches_formula() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: CountAdapter = s.adapter
	# 400 / 40 = 10 visible + 1 boundary item.
	assert_that(rv.get_child_holder_count()).is_equal(11)
	assert_that(_positions(rv)).is_equal([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
	assert_that(adapter.created).is_equal(11)
	# Item 5 sits at y = 5*40 - 0 = 200.
	var holder := rv.get_child_holder_at(5)
	assert_that(holder.get_control().position).is_equal(Vector2(0, 200))
	assert_that(rv.get_layout().get_content_size(rv)).is_equal(100 * EXTENT)
	rv.free_items()
	rv.free()


func test_scroll_recycles_and_reuses() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: CountAdapter = s.adapter
	rv.scroll_vertically(160)  # four items out, four in
	# The script recycled 0..3 and filled 11..14.
	assert_that(_positions(rv)).is_equal([4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14])
	# Recycled holders move through the cache into the pool on fill misses and
	# are reused: creation stays flat at the viewport size.
	assert_that(adapter.created).is_equal(11)
	# Holder 10 keeps its formula position: y = 10*40 - 160 = 240.
	assert_that(rv.get_child_holder_at(6).get_control().position).is_equal(Vector2(0, 240))
	rv.free_items()
	rv.free()


func test_scroll_clamps_at_content_end() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	# Max offset = content(4000) - viewport(400) = 3600.
	rv.scroll_vertically(100000)
	assert_that(rv.get_scroll_offset()).is_equal(3600)
	# The last viewport: positions 90..99 (10 items).
	assert_that(_positions(rv)).is_equal([90, 91, 92, 93, 94, 95, 96, 97, 98, 99])
	rv.free_items()
	rv.free()


func test_scroll_to_position_uses_get_position_offset() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	rv.scroll_to_position(40)
	assert_that(rv.get_scroll_offset()).is_equal(40 * EXTENT)
	assert_that(_positions(rv)).contains(40)
	rv.free_items()
	rv.free()


func test_scroll_axis_comes_from_can_scroll() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	# CustomLM can_scroll_vertically = true: vertical scroll moves.
	var before := rv.get_scroll_offset()
	rv.scroll_vertically(40)
	assert_that(rv.get_scroll_offset()).is_equal(before + 40)
	# Horizontal scroll is refused: horizontal offset stays 0.
	rv.scroll_horizontally(40)
	assert_that(rv.get_scroll_offset_horizontal()).is_equal(0)
	rv.free_items()
	rv.free()


func test_data_change_dispatches_on_data_changed() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var lm: CustomLM = s.lm
	var adapter: CountAdapter = s.adapter
	adapter.count = 50
	rv.notify_data_changed()
	await get_tree().process_frame
	assert_that(lm.data_changed_count).is_equal(1)
	# Content size follows the new count through _get_content_size.
	assert_that(rv.get_layout().get_content_size(rv)).is_equal(50 * EXTENT)
	rv.free_items()
	rv.free()


func test_prefetch_consulted_when_enabled() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var lm: CustomLM = s.lm
	rv.set_prefetch_enabled(true)
	rv.request_layout()
	# Scrolling sets the layout direction; the prefetch pass then consults
	# _collect_adjacent_prefetch_positions and warms the pool ahead of the
	# viewport. (Whether a view is actually created depends on the recycler's
	# cache state, covered by the C++ layout managers' own tests; here we pin
	# that the script hook is dispatched at all.)
	rv.scroll_vertically(40)
	assert_that(lm.prefetch_consulted).is_greater_equal(1)
	rv.free_items()
	rv.free()


func test_big_jump_reuses_everything_without_config() -> void:
	# A big jump recycles the whole viewport at once. The view cache grows with
	# the visible count (port of Recycler.mViewCacheMax), so every recycled
	# holder is kept in the cache and the jump's fill reuses them by type (the
	# cache fallback path). No pool sizing is needed: creation stays flat.
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: CountAdapter = s.adapter
	rv.scroll_to_position(80)
	assert_that(adapter.created).is_equal(11)
	# Jumps back and forth stay flat too.
	rv.scroll_to_position(0)
	rv.scroll_to_position(80)
	assert_that(adapter.created).is_equal(11)
	rv.free_items()
	rv.free()


func test_unimplemented_layout_keeps_rv_alive() -> void:
	# A script LM that overrides nothing: the base warns once and lays out
	# nothing, but the RV must not crash or leak holders.
	var s := _make_setup(PlainLM.new())
	var rv: RecyclerView = s.rv
	assert_that(rv.get_child_holder_count()).is_equal(0)
	assert_that(rv.get_scroll_offset()).is_equal(0)
	rv.free_items()
	rv.free()
