# Tests for the ViewHolder + Adapter ports (GDScript-facing API).

extends GdUnitTestSuite

class MyAdapter extends Adapter:
	var items: Array = []
	var bind_calls: Array = []
	var create_calls: Array = []

	func _get_item_count() -> int:
		return items.size()

	func _create_item(parent: Control, view_type: int) -> ViewHolder:
		create_calls.append([view_type])
		return ViewHolder.new()

	func _bind_item(holder: ViewHolder, position: int) -> void:
		bind_calls.append([position, holder.get_layout_position()])

	func _get_item_view_type(position: int) -> int:
		return 1

	func _get_item_id(position: int) -> int:
		return items[position]


class MyObserver extends AdapterDataObserver:
	var events: Array = []

	func _on_changed() -> void:
		events.append(["changed"])

	func _on_item_range_inserted(position: int, count: int) -> void:
		events.append(["inserted", position, count])

	func _on_item_range_removed(position: int, count: int) -> void:
		events.append(["removed", position, count])

	func _on_item_range_changed(position: int, count: int, payload) -> void:
		events.append(["changed_range", position, count])

	func _on_item_moved(from_position: int, to_position: int) -> void:
		events.append(["moved", from_position, to_position])


func test_view_holder_core_state() -> void:
	var vh := ViewHolder.new()
	var ctrl := Control.new()
	vh.set_control(ctrl)
	assert_that(vh.get_control()).is_same(ctrl)
	assert_that(vh.get_item_view_type()).is_equal(-1)
	assert_that(vh.get_layout_position()).is_equal(-1)

	vh.set_item_view_type(2)
	vh.set_position(5)
	vh.set_stable_id(42)
	assert_that(vh.get_item_view_type()).is_equal(2)
	assert_that(vh.get_layout_position()).is_equal(5)
	assert_that(vh.get_stable_id()).is_equal(42)

	# Bound/update/invalid flags.
	vh.set_flags(ViewHolder.FLAG_BOUND, ViewHolder.FLAG_BOUND | ViewHolder.FLAG_UPDATE | ViewHolder.FLAG_INVALID)
	assert_that(vh.is_bound()).is_true()
	assert_that(vh.is_invalid()).is_false()
	assert_that(vh.is_updated()).is_false()
	ctrl.free()


func test_view_holder_recyclable() -> void:
	var vh := ViewHolder.new()
	assert_that(vh.is_recyclable()).is_true()
	vh.set_is_recyclable(false)
	assert_that(vh.is_recyclable()).is_false()
	vh.set_is_recyclable(true)
	assert_that(vh.is_recyclable()).is_true()


func test_view_holder_offset_and_reset() -> void:
	var vh := ViewHolder.new()
	vh.set_position(3)
	vh.set_flags(ViewHolder.FLAG_BOUND, ViewHolder.FLAG_BOUND)
	vh.set_is_recyclable(false)
	vh.set_stable_id(7)
	vh.offset_position(2, true)
	assert_that(vh.get_layout_position()).is_equal(5)
	assert_that(vh.get_old_position()).is_equal(3)

	vh.reset_internal()
	assert_that(vh.is_bound()).is_false()
	assert_that(vh.get_layout_position()).is_equal(-1)
	assert_that(vh.is_recyclable()).is_true()


func test_adapter_create_and_bind() -> void:
	var adapter := MyAdapter.new()
	adapter.items = [10, 20, 30]

	var holder := adapter.create_view_holder(null, 5)
	assert_that(holder).is_not_null()
	assert_that(holder.get_item_view_type()).is_equal(5)
	assert_that(adapter.create_calls.size()).is_equal(1)

	adapter.bind_view_holder(holder, 1)
	assert_that(holder.get_position()).is_equal(1)
	assert_that(holder.is_bound()).is_true()
	assert_that(adapter.bind_calls.size()).is_equal(1)
	assert_that(adapter.bind_calls[0][0]).is_equal(1)


func test_adapter_stable_ids() -> void:
	var adapter := MyAdapter.new()
	adapter.items = [10, 20]
	adapter.set_has_stable_ids(true)
	assert_that(adapter.has_stable_ids()).is_true()
	var holder := adapter.create_view_holder(null, 0)
	adapter.bind_view_holder(holder, 1)
	assert_that(holder.get_stable_id()).is_equal(20)


func test_adapter_notify_dispatches_to_observers() -> void:
	var adapter := MyAdapter.new()
	var observer := MyObserver.new()
	adapter.register_adapter_data_observer(observer)
	assert_that(adapter.has_observers()).is_true()

	adapter.notify_item_inserted(3)
	adapter.notify_item_removed(2)
	adapter.notify_item_moved(1, 4)
	adapter.notify_data_set_changed()

	assert_that(observer.events.size()).is_equal(4)
	assert_that(observer.events[0]).is_equal(["inserted", 3, 1])
	assert_that(observer.events[1]).is_equal(["removed", 2, 1])
	assert_that(observer.events[2]).is_equal(["moved", 1, 4])
	assert_that(observer.events[3]).is_equal(["changed"])

	adapter.unregister_adapter_data_observer(observer)
	assert_that(adapter.has_observers()).is_false()
	adapter.notify_item_inserted(0)
	assert_that(observer.events.size()).is_equal(4)


func test_adapter_set_has_stable_ids_rejected_with_observers() -> void:
	var adapter := MyAdapter.new()
	var observer := MyObserver.new()
	adapter.register_adapter_data_observer(observer)
	adapter.set_has_stable_ids(true)
	# Rejected because observers are registered.
	assert_that(adapter.has_stable_ids()).is_false()
