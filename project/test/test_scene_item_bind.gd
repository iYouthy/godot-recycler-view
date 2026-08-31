# Tests that scene-based items are bound only after their ready pass.
#
# An item whose root comes from a .tscn often refreshes itself through
# @onready references (e.g. `@onready var label = $Label`). Binding the holder
# before its control enters the tree runs _bind_item against a not-yet-ready
# scene: the references are null and the item ends up empty. The RecyclerView
# must mount the control (running the ready pass) before calling _bind_item —
# never requiring the adapter author to `await ctrl.ready` manually.

extends GdUnitTestSuite

const ITEM_SCENE := preload("res://test_list_item.tscn")


# Binds through the item scene's refresh(), which touches an @onready Label.
class SceneAdapter extends Adapter:
	var items: Array[String] = []
	var created := 0
	var bound_inside_tree := true

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		created += 1
		var vh := ViewHolder.new()
		vh.set_control(ITEM_SCENE.instantiate())
		return vh

	func _bind_item(holder: ViewHolder, position: int) -> void:
		var ctrl: TestListItem = holder.get_control()
		bound_inside_tree = bound_inside_tree and ctrl.is_inside_tree()
		ctrl.refresh(items[position])


func _make_setup(count := 5) -> Dictionary:
	get_window().size = Vector2i(1920, 1080)
	get_window().content_scale_size = Vector2i(1920, 1080)
	await get_tree().process_frame
	var rv := RecyclerView.new()
	rv.position = Vector2(0, 0)
	rv.set_size(Vector2(300, 300))
	rv.set_item_extent(60)
	rv.set_prefetch_enabled(true)
	# Mount before wiring the adapter: the first layout must run inside the
	# tree so first-mount binds wait for the item's ready pass. (An off-tree
	# RV lays out synchronously and binds immediately, as before — fine for
	# code-built items, wrong for scene items whose @onready refs need the
	# ready pass.)
	get_tree().root.add_child(rv)
	var adapter := SceneAdapter.new()
	for i in count:
		adapter.items.append("item %d" % i)
	rv.set_adapter(adapter)
	rv.set_layout(LinearLayoutManager.new())
	await get_tree().process_frame
	return { "rv": rv, "adapter": adapter }


func _label_text(rv: RecyclerView, position: int) -> String:
	for i in rv.get_child_holder_count():
		var holder := rv.get_child_holder_at(i)
		if holder.get_position() == position:
			return (holder.get_control().get_node("Label") as Label).text
	return ""


func test_first_layout_binds_scene_items_after_ready() -> void:
	var s := await _make_setup()
	var rv: RecyclerView = s.rv
	var adapter: SceneAdapter = s.adapter
	# _bind_item ran with the control inside the tree, so @onready refs were up.
	assert_that(adapter.bound_inside_tree).is_true()
	for i in 3:
		assert_that(_label_text(rv, i)).is_equal("item %d" % i)
	rv.free_items()
	rv.free()


func test_reused_holder_binds_again_after_ready() -> void:
	# Regression: a holder recycled by scrolling is re-used for a new position;
	# the re-bind must also run after the control re-enters the tree.
	var s := await _make_setup(20)
	var rv: RecyclerView = s.rv
	var adapter: SceneAdapter = s.adapter
	# Scroll past the first rows so the top holders recycle...
	rv.scroll_vertically(300)
	await get_tree().process_frame
	rv.scroll_vertically(-300)
	await get_tree().process_frame
	# ...and every visible row must still show its own content.
	for i in rv.get_child_holder_count():
		var pos: int = rv.get_child_holder_at(i).get_position()
		assert_that(_label_text(rv, pos)).is_equal("item %d" % pos)
	rv.free_items()
	rv.free()


func test_scroll_into_prefetched_rows_keeps_content() -> void:
	# Prefetch creates fresh holders before they scroll into view; those too
	# must be bound only after the mount (they have never been ready before).
	var s := await _make_setup(30)
	var rv: RecyclerView = s.rv
	# Drag far enough that prefetched rows enter the viewport.
	for i in 20:
		rv.scroll_vertically(15)
		await get_tree().process_frame
	for i in rv.get_child_holder_count():
		var pos: int = rv.get_child_holder_at(i).get_position()
		assert_that(_label_text(rv, pos)).is_equal("item %d" % pos)
	rv.free_items()
	rv.free()
