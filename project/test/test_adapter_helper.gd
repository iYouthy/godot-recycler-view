# Tests for the AdapterHelper one-pass update port.

extends GdUnitTestSuite


func test_insert_offsets_holders() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_inserted(3, 2)
	var before := ViewHolder.new()
	before.set_position(2)
	var at := ViewHolder.new()
	at.set_position(3)
	var after := ViewHolder.new()
	after.set_position(7)
	helper.apply_updates_to_holder(before)
	helper.apply_updates_to_holder(at)
	helper.apply_updates_to_holder(after)
	assert_that(before.get_position()).is_equal(2)  # below the insert: unchanged
	assert_that(at.get_position()).is_equal(5)      # at/above the insert: +count
	assert_that(after.get_position()).is_equal(9)
	assert_that(before.is_removed()).is_false()
	assert_that(before.is_updated()).is_false()


func test_remove_offsets_and_marks_removed() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_removed(2, 3)
	var below := ViewHolder.new()
	below.set_position(1)
	var removed := ViewHolder.new()
	removed.set_position(3)
	var above := ViewHolder.new()
	above.set_position(6)
	helper.apply_updates_to_holder(below)
	helper.apply_updates_to_holder(removed)
	helper.apply_updates_to_holder(above)
	assert_that(below.get_position()).is_equal(1)
	assert_that(removed.is_removed()).is_true()
	assert_that(above.get_position()).is_equal(3)  # 6 - count


func test_move_offsets_holders() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_moved(2, 5)  # move forward
	var moved := ViewHolder.new()
	moved.set_position(2)
	var between := ViewHolder.new()
	between.set_position(4)
	var outside := ViewHolder.new()
	outside.set_position(7)
	helper.apply_updates_to_holder(moved)
	helper.apply_updates_to_holder(between)
	helper.apply_updates_to_holder(outside)
	assert_that(moved.get_position()).is_equal(5)
	assert_that(between.get_position()).is_equal(3)  # in-between shifted down
	assert_that(outside.get_position()).is_equal(7)

	var helper_back := AdapterHelper.new()
	helper_back.on_item_range_moved(5, 2)  # move backward
	var moved_back := ViewHolder.new()
	moved_back.set_position(5)
	var between_back := ViewHolder.new()
	between_back.set_position(3)
	helper_back.apply_updates_to_holder(moved_back)
	helper_back.apply_updates_to_holder(between_back)
	assert_that(moved_back.get_position()).is_equal(2)
	assert_that(between_back.get_position()).is_equal(4)  # in-between shifted up


func test_change_marks_updated_without_moving() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_changed(1, 2, "payload")
	var hit := ViewHolder.new()
	hit.set_position(2)
	var miss := ViewHolder.new()
	miss.set_position(5)
	helper.apply_updates_to_holder(hit)
	helper.apply_updates_to_holder(miss)
	assert_that(hit.is_updated()).is_true()
	assert_that(hit.get_position()).is_equal(2)
	assert_that(miss.is_updated()).is_false()


func test_apply_pending_updates_to_position() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_inserted(3, 2)
	assert_that(helper.apply_pending_updates_to_position(3)).is_equal(5)
	helper.clear()

	helper.on_item_range_removed(3, 2)
	assert_that(helper.apply_pending_updates_to_position(3)).is_equal(-1)
	assert_that(helper.apply_pending_updates_to_position(5)).is_equal(3)
	helper.clear()


func test_pending_updates_queue_and_clear() -> void:
	var helper := AdapterHelper.new()
	helper.on_item_range_inserted(1, 1)
	assert_that(helper.has_pending_updates()).is_true()
	helper.on_item_range_removed(0, 1)
	assert_that(helper.has_pending_updates()).is_true()
	helper.clear()
	assert_that(helper.has_pending_updates()).is_false()

	# Empty ranges and no-op moves are rejected.
	var noop := AdapterHelper.new()
	assert_that(noop.on_item_range_inserted(1, 0)).is_false()
	assert_that(noop.on_item_range_removed(1, -1)).is_false()
	assert_that(noop.on_item_range_moved(3, 3)).is_false()
	assert_that(noop.has_pending_updates()).is_false()
