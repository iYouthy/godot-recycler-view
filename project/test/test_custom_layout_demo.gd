# Tests for custom_layout_demo's layout: the wave rows are laid out by the
# GDScript CustomLayoutManager, creation stays bounded (recycle-reuse works
# through the script's own remove/recycle calls), and scroll_to_position lands
# on the position via _get_position_offset.

extends GdUnitTestSuite

const CustomLayoutManagerScript := preload("res://custom_layout_manager.gd")
const CustomLayoutAdapterScript := preload("res://custom_layout_adapter.gd")


func _make_setup() -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(200, 480))
	rv.set_item_extent(40)
	rv.set_prefetch_enabled(false)
	var lm := CustomLayoutManagerScript.new()
	var adapter := CustomLayoutAdapterScript.new()
	rv.set_adapter(adapter)
	rv.set_layout(lm)
	rv.request_layout()
	get_tree().root.add_child(rv)
	await get_tree().process_frame
	return { "rv": rv, "lm": lm, "adapter": adapter }


func _positions(rv: RecyclerView) -> Array:
	var out := []
	for i in rv.get_child_holder_count():
		out.append(rv.get_child_holder_at(i).get_position())
	return out


func test_wave_layout_places_rows() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var lm: CustomLayoutManagerScript = s.lm
	# 480 / 40 = 12 visible + 1 boundary.
	assert_that(rv.get_child_holder_count()).is_equal(13)
	assert_that(_positions(rv)).is_equal([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
	# The wave: row 0 sits at x = sin(0) * 24 = 0, row 1 at sin(0.2)*24 ≈ 4.77.
	assert_that(rv.get_child_holder_at(0).get_control().position.x).is_equal(0.0)
	assert_that(rv.get_child_holder_at(1).get_control().position.x).is_greater(4.0)
	assert_that(rv.get_child_holder_at(1).get_control().position.x).is_less(6.0)
	# Content size drives the scroll range.
	assert_that(lm.get_content_size(rv)).is_equal(200 * 40)
	rv.free_items()
	rv.free()


func test_scrolling_reuses_wave_rows() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: CustomLayoutAdapterScript = s.adapter
	rv.scroll_vertically(200)
	assert_that(_positions(rv)).is_equal([5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17])
	# Recycled rows move through the cache into the pool on fill misses and are
	# reused: the created counter stays at the viewport size.
	assert_that(adapter.created).is_equal(13)
	rv.scroll_to_position(100)
	assert_that(rv.get_scroll_offset()).is_equal(100 * 40)
	assert_that(_positions(rv)).contains(100)
	# Big jumps recycle the whole viewport at once; with the pool sized for a
	# full viewport the swap reuses every holder — creation stays flat.
	var created_before_jump := adapter.created
	rv.scroll_to_position(40)
	rv.scroll_to_position(150)
	assert_that(adapter.created).is_equal(created_before_jump)
	rv.free_items()
	rv.free()
