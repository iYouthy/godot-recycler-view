# Tests for the ItemDecoration port (insets, drawing).

extends GdUnitTestSuite


class PlainAdapter extends Adapter:
	var count: int = 0

	func _get_item_count() -> int:
		return count

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		var vh := ViewHolder.new()
		vh.set_control(Control.new())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		pass


class BottomDecor extends ItemDecoration:
	func _get_item_offsets(position: int, parent: Control) -> Vector4:
		return Vector4(0, 0, 0, 8)


class LeftDecor extends ItemDecoration:
	func _get_item_offsets(position: int, parent: Control) -> Vector4:
		return Vector4(3, 0, 0, 0)


class AllSidesDecor extends ItemDecoration:
	func _get_item_offsets(position: int, parent: Control) -> Vector4:
		return Vector4(2, 3, 4, 5)


class DrawDecor extends ItemDecoration:
	var draws := 0

	func _get_item_offsets(position: int, parent: Control) -> Vector4:
		return Vector4()

	func _on_draw(parent: Control) -> void:
		draws += 1
		var rv: RecyclerView = parent
		rv.get_decorated_item_rect(0)  # rect lookup must be safe here


func _make_setup() -> Dictionary:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(200, 600))
	var adapter := PlainAdapter.new()
	adapter.count = 10
	rv.set_item_extent(60)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	rv.request_layout()
	return { "rv": rv, "adapter": adapter }


func test_insets_apply_to_layout() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	rv.add_item_decoration(BottomDecor.new())
	rv.request_layout()

	# The item height shrinks by the bottom inset; the next item keeps its base
	# slot, leaving an 8px gap for the divider.
	var c0: Control = rv.get_child_holder_at(0).get_control()
	assert_that(int(c0.size.y)).is_equal(52)
	var c1: Control = rv.get_child_holder_at(1).get_control()
	assert_that(int(c1.position.y)).is_equal(60)
	# Content size is unaffected by insets (10 items * 60).
	var layout: LinearLayoutManager = rv.get_layout()
	assert_that(layout.get_content_size(rv)).is_equal(600)
	rv.free_items()
	rv.free()


func test_multiple_decorations_accumulate() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	rv.add_item_decoration(BottomDecor.new())
	rv.add_item_decoration(LeftDecor.new())
	rv.request_layout()

	assert_that(rv.get_item_insets(0)).is_equal(Vector4(3, 0, 0, 8))
	assert_that(rv.get_item_decoration_count()).is_equal(2)
	var c0: Control = rv.get_child_holder_at(0).get_control()
	assert_that(int(c0.position.x)).is_equal(3)
	assert_that(int(c0.size.x)).is_equal(197)
	assert_that(int(c0.size.y)).is_equal(52)
	rv.free_items()
	rv.free()


func test_all_sides_insets() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	rv.add_item_decoration(AllSidesDecor.new())
	rv.request_layout()

	assert_that(rv.get_item_insets(0)).is_equal(Vector4(2, 3, 4, 5))
	var c0: Control = rv.get_child_holder_at(0).get_control()
	assert_that(int(c0.position.x)).is_equal(2)
	assert_that(int(c0.position.y)).is_equal(3)
	assert_that(int(c0.size.x)).is_equal(194)  # 200 - 2 - 4
	assert_that(int(c0.size.y)).is_equal(52)   # 60 - 3 - 5
	rv.free_items()
	rv.free()


func test_on_draw_forwards() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	var decor := DrawDecor.new()
	rv.add_item_decoration(decor)
	rv.request_layout()

	# headless mode never runs the engine's draw pass, so dispatch on_draw
	# directly; in the editor RecyclerView._draw drives it.
	decor.on_draw(rv)
	assert_that(decor.draws).is_equal(1)
	rv.free_items()
	rv.free()


func test_get_decorated_item_rect_matches_item() -> void:
	var s := _make_setup()
	var rv: RecyclerView = s.rv
	rv.add_item_decoration(BottomDecor.new())
	rv.request_layout()

	var rect := rv.get_decorated_item_rect(0)
	var c0: Control = rv.get_child_holder_at(0).get_control()
	assert_that(rect.position).is_equal(c0.position)
	assert_that(rect.size).is_equal(c0.size)
	assert_that(int(rect.size.y)).is_equal(52)
	rv.free_items()
	rv.free()


func test_grid_with_divider() -> void:
	var rv := RecyclerView.new()
	rv.set_size(Vector2(360, 600))
	var adapter := PlainAdapter.new()
	adapter.count = 12
	rv.set_item_extent(60)
	rv.set_adapter(adapter)
	var layout := GridLayoutManager.new()
	layout.set_span_count(3)
	rv.set_layout(layout)
	rv.add_item_decoration(BottomDecor.new())
	rv.request_layout()

	# Row 0 cell height shrinks by the bottom inset; width and row slot stay.
	var c0: Control = rv.get_child_holder_at(0).get_control()
	assert_that(int(c0.size.y)).is_equal(52)
	assert_that(int(c0.size.x)).is_equal(120)
	# Row 1 still starts at y=60 (row height is unaffected by insets).
	var c3: Control = rv.get_child_holder_at(3).get_control()
	assert_that(int(c3.position.y)).is_equal(60)
	assert_that(int(c3.size.y)).is_equal(52)
	rv.free_items()
	rv.free()
