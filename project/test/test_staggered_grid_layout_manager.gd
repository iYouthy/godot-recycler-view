# Tests for the StaggeredGridLayoutManager masonry layout: shortest-column
# assignment, per-column accumulation, content size, and virtualization.

extends GdUnitTestSuite


class CellAdapter extends Adapter:
	var count: int = 0
	var heights: Array = []
	var created := 0

	func _get_item_count() -> int:
		return count

	func _get_item_extent(position: int) -> int:
		return heights[position] if position < heights.size() else 60

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		var label := Label.new()
		label.set_size(Vector2(100, 100))
		vh.set_control(label)
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		(holder.get_control() as Label).text = str(position)


func _make_rv(heights: Array, count: int, span: int, prefetch: bool = true) -> RecyclerView:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(360, 600))
	var adapter := CellAdapter.new()
	adapter.heights = heights
	adapter.count = count
	rv.set_adapter(adapter)
	var layout := StaggeredGridLayoutManager.new()
	layout.set_span_count(span)
	rv.set_layout(layout)
	rv.set_prefetch_enabled(prefetch)
	rv.request_layout()
	get_tree().root.add_child(rv)
	return rv


func _holder_at(rv: RecyclerView, position: int) -> ViewHolder:
	for i in rv.get_child_holder_count():
		var h: ViewHolder = rv.get_child_holder_at(i)
		if h.get_position() == position:
			return h
	return null


func test_staggered_shortest_column_assignment() -> void:
	var heights := [60, 120, 90, 80, 100, 110]
	var rv := await _make_rv(heights, 6, 3)
	var layout: StaggeredGridLayoutManager = rv.get_layout()
	# Each item flows into the currently shortest column (first wins ties).
	assert_that(layout.get_item_column(0)).is_equal(0)
	assert_that(layout.get_item_column(1)).is_equal(1)
	assert_that(layout.get_item_column(2)).is_equal(2)
	assert_that(layout.get_item_column(3)).is_equal(0)  # col 0 shortest (60)
	assert_that(layout.get_item_column(4)).is_equal(2)  # col 2 shortest (90)
	assert_that(layout.get_item_column(5)).is_equal(1)  # col 1 shortest (120)
	# Column tops accumulate independently.
	assert_that(layout.get_col_top_of_position(0)).is_equal(0)
	assert_that(layout.get_col_top_of_position(3)).is_equal(60)
	assert_that(layout.get_col_top_of_position(4)).is_equal(90)
	assert_that(layout.get_col_top_of_position(5)).is_equal(120)
	# Content size is the tallest column's end.
	assert_that(layout.get_content_size(rv)).is_equal(230)
	rv.free_items()
	rv.free()


func test_staggered_positions_are_staggered() -> void:
	var heights := [60, 120, 90, 80, 100, 110]
	var rv := await _make_rv(heights, 6, 3)
	# 3 columns of 120px in a 360px viewport; items sit at their column's top,
	# NOT row-aligned (the staggered look).
	var h3: ViewHolder = _holder_at(rv, 3)
	assert_that(int(h3.get_control().position.x)).is_equal(0)  # column 0
	assert_that(int(h3.get_control().position.y)).is_equal(60)
	var h4: ViewHolder = _holder_at(rv, 4)
	assert_that(int(h4.get_control().position.x)).is_equal(240)  # column 2
	assert_that(int(h4.get_control().position.y)).is_equal(90)
	var h5: ViewHolder = _holder_at(rv, 5)
	assert_that(int(h5.get_control().position.x)).is_equal(120)  # column 1
	assert_that(int(h5.get_control().position.y)).is_equal(120)
	rv.free_items()
	rv.free()


func test_staggered_scroll_virtualizes_without_overlap() -> void:
	# 100 items of varying heights; virtualization keeps creation well below a
	# full rebuild, and no two items in a column overlap while scrolling.
	var heights := []
	for i in 100:
		heights.append(50 + (i * 37) % 101)
	var rv := await _make_rv(heights, 100, 3, false)
	var adapter: CellAdapter = rv.get_adapter()

	for i in 60:
		rv.scroll_vertically(40)
		var by_col := {}
		for j in rv.get_child_holder_count():
			var h: ViewHolder = rv.get_child_holder_at(j)
			var c: Control = h.get_control()
			var col := int(c.position.x / 120)
			if not by_col.has(col):
				by_col[col] = []
			by_col[col].append([c.position.y, c.position.y + c.size.y])
		for col in by_col:
			var ys: Array = by_col[col]
			ys.sort()
			for k in range(1, ys.size()):
				assert_that(ys[k][0] >= ys[k - 1][1] - 1.0).is_true()  # no overlap in a column

	# Virtualization: far fewer holders than a full 100-item rebuild.
	assert_that(adapter.created).is_less(100)
	rv.free_items()
	rv.free()


func test_staggered_span_change_rebuilds_columns() -> void:
	var heights := [60, 120, 90, 80, 100, 110]
	var rv := await _make_rv(heights, 6, 3)
	var layout: StaggeredGridLayoutManager = rv.get_layout()
	assert_that(layout.get_item_column(4)).is_equal(2)

	layout.set_span_count(2)
	rv.request_layout()
	# 2 columns: items alternate into the shorter column.
	var col0_tops := []
	var col1_tops := []
	for pos in 6:
		if layout.get_item_column(pos) == 0:
			col0_tops.append(layout.get_col_top_of_position(pos))
		else:
			col1_tops.append(layout.get_col_top_of_position(pos))
	# Each column's tops are strictly increasing (no overlap within a column).
	for i in range(1, col0_tops.size()):
		assert_that(col0_tops[i]).is_greater(col0_tops[i - 1])
	for i in range(1, col1_tops.size()):
		assert_that(col1_tops[i]).is_greater(col1_tops[i - 1])
	rv.free_items()
	rv.free()


func test_staggered_get_position_offset_is_column_top() -> void:
	var heights := [60, 120, 90, 80, 100, 110]
	var rv := await _make_rv(heights, 6, 3)
	var layout: StaggeredGridLayoutManager = rv.get_layout()
	assert_that(layout.get_position_offset(0)).is_equal(0)
	assert_that(layout.get_position_offset(3)).is_equal(60)
	assert_that(layout.get_position_offset(5)).is_equal(120)
	rv.free_items()
	rv.free()
