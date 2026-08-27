# Port of androidx DiffUtilTest.kt exercising the GDScript-facing API
# (DiffUtil / DiffUtilCallback / DiffResult / ListUpdateCallback).

extends GdUnitTestSuite

const NO_POSITION := -1

var _id_counter := 0

class Item:
	var id: int
	var new_item: bool
	var changed: bool
	var payload: Variant
	var data: String

	func _init(p_id: int, p_new_item: bool, p_data: String = "") -> void:
		id = p_id
		new_item = p_new_item
		changed = false
		payload = null
		data = p_data

	func equals(other: Item) -> bool:
		return id == other.id \
			and new_item == other.new_item \
			and changed == other.changed \
			and payload == other.payload \
			and data == other.data


class TestCallback extends DiffUtilCallback:
	var old_list: Array
	var new_list: Array

	func _get_old_list_size() -> int:
		return old_list.size()

	func _get_new_list_size() -> int:
		return new_list.size()

	func _are_items_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_list[old_item_position].id == new_list[new_item_position].id

	func _are_contents_the_same(old_item_position: int, new_item_position: int) -> bool:
		return old_list[old_item_position].data == new_list[new_item_position].data

	func _get_change_payload(old_item_position: int, new_item_position: int) -> Variant:
		return new_list[new_item_position].payload


class ApplyingCallback extends ListUpdateCallback:
	var target: Array
	var id_state: Dictionary
	var violations: Array = []

	func _init(p_target: Array, p_id_state: Dictionary) -> void:
		target = p_target
		id_state = p_id_state

	func _on_inserted(position: int, count: int) -> void:
		for i in count:
			id_state["next"] += 1
			var item := Item.new(id_state["next"], true, "new_%d" % id_state["next"])
			target.insert(position + i, item)

	func _on_removed(position: int, count: int) -> void:
		for i in count:
			target.remove_at(position)

	func _on_moved(from_position: int, to_position: int) -> void:
		var item: Item = target[from_position]
		target.remove_at(from_position)
		target.insert(to_position, item)

	func _on_changed(position: int, count: int, payload: Variant) -> void:
		for i in count:
			var existing: Item = target[position + i]
			if existing.changed or existing.new_item or existing.payload != null:
				violations.append([position + i, existing.changed, existing.new_item, existing.payload])
			existing.changed = true
			existing.payload = payload


func _make_item(p_new_item: bool) -> Item:
	_id_counter += 1
	return Item.new(_id_counter, p_new_item, "data_%d" % _id_counter)


func _init_with_size(p_size: int) -> Array:
	_id_counter = 0
	var list: Array = []
	for i in p_size:
		list.append(_make_item(false))
	return list


# ------------------------------------------------------------------
# Verification (port of check() + assertEquals in DiffUtilTest.kt).

func _compute_expected_new_items_for_existing(p_after: Array) -> Dictionary:
	var duplicate_diffs := {}
	for item in p_after:
		if not item.new_item:
			duplicate_diffs[item.id] = 1 + (duplicate_diffs.get(item.id, 1))
	for item in _last_before:
		duplicate_diffs[item.id] = duplicate_diffs.get(item.id, 0) - 1
	return duplicate_diffs


var _last_before: Array = []


func _verify_applied(p_applied: Array, p_after: Array) -> void:
	assert_that(p_applied.size()).is_equal(p_after.size())
	var duplicate_diffs := _compute_expected_new_items_for_existing(p_after)
	for i in p_after.size():
		var item: Item = p_applied[i]
		var expected: Item = p_after[i]
		if expected.new_item:
			assert_that(item.new_item).is_true()
		elif duplicate_diffs.get(expected.id, 0) > 0 and item.new_item:
			duplicate_diffs[expected.id] -= 1
		elif expected.changed:
			assert_that(item.new_item).is_false()
			assert_that(item.changed).is_true()
			assert_that(item.id).is_equal(expected.id)
			assert_that(item.payload).is_equal(expected.payload)
		else:
			assert_that(item.equals(expected)).is_true()


func _check(p_before: Array, p_after: Array) -> void:
	_last_before = p_before
	var callback := TestCallback.new()
	callback.old_list = p_before
	callback.new_list = p_after

	var result := DiffUtil.calculate_diff(callback)
	var applied: Array = p_before.duplicate()
	var id_state := { "next": _id_counter }
	var applier := ApplyingCallback.new(applied, id_state)
	result.dispatch_updates_to(applier)
	assert_that(applier.violations).is_empty()
	_verify_applied(applied, p_after)

	# Position conversion: every mapped pair must share the same id, and removed
	# items must be absent from the other list.
	var missing_before := []
	var after_copy: Array = p_after.duplicate()
	for old_pos in p_before.size():
		var new_pos := result.convert_old_position_to_new(old_pos)
		if new_pos != NO_POSITION:
			assert_that(p_before[old_pos].id).is_equal(p_after[new_pos].id)
			after_copy.erase(p_after[new_pos])
		else:
			missing_before.append(old_pos)
	for old_pos in missing_before:
		assert_that(after_copy.has(p_before[old_pos])).is_false()
	assert_that(result.convert_old_position_to_new(p_before.size())).is_equal(NO_POSITION)
	assert_that(result.convert_old_position_to_new(-1)).is_equal(NO_POSITION)

	var missing_after := []
	var before_copy: Array = p_before.duplicate()
	for new_pos in p_after.size():
		var old_pos := result.convert_new_position_to_old(new_pos)
		if old_pos != NO_POSITION:
			assert_that(p_after[new_pos].id).is_equal(p_before[old_pos].id)
			before_copy.erase(p_before[old_pos])
		else:
			missing_after.append(new_pos)
	for new_pos in missing_after:
		assert_that(before_copy.has(p_after[new_pos])).is_false()
	assert_that(result.convert_new_position_to_old(p_after.size())).is_equal(NO_POSITION)
	assert_that(result.convert_new_position_to_old(-1)).is_equal(NO_POSITION)


# ------------------------------------------------------------------
# Operations on the "after" list.

func _add(p_after: Array, p_index: int) -> void:
	p_after.insert(p_index, _make_item(true))


func _delete(p_after: Array, p_index: int) -> void:
	p_after.remove_at(p_index)


func _move(p_after: Array, p_from: int, p_to: int) -> void:
	var item: Item = p_after[p_from]
	p_after.remove_at(p_from)
	p_after.insert(p_to, item)


func _update(p_after: Array, p_index: int) -> void:
	var existing: Item = p_after[p_index]
	if existing.new_item:
		return
	existing.changed = true
	existing.payload = null
	_id_counter += 1
	existing.data = "data_%d" % _id_counter


func _update_with_payload(p_after: Array, p_index: int) -> void:
	var existing: Item = p_after[p_index]
	if existing.new_item:
		return
	existing.changed = true
	existing.payload = "payload_%d" % _id_counter
	_id_counter += 1
	existing.data = "data_%d" % _id_counter


func _duplicate(p_after: Array, p_pos: int, p_to: int) -> void:
	var item: Item = p_after[p_pos]
	p_after.insert(p_pos, item)


# ------------------------------------------------------------------
# Test cases.

func test_no_change() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_check(before, after)


func test_add_items() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_add(after, 1)
	_check(before, after)


func test_gen2() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_add(after, 5)
	_delete(after, 3)
	_delete(after, 1)
	_check(before, after)


func test_gen3() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_add(after, 0)
	_delete(after, 1)
	_delete(after, 3)
	_check(before, after)


func test_gen4() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_add(after, 5)
	_add(after, 1)
	_add(after, 4)
	_add(after, 4)
	_check(before, after)


func test_gen5() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_delete(after, 0)
	_delete(after, 2)
	_add(after, 0)
	_add(after, 2)
	_check(before, after)


func test_gen6() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_delete(after, 0)
	_delete(after, 0)
	_check(before, after)


func test_gen7() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_move(after, 2, 0)
	_delete(after, 2)
	_add(after, 2)
	_check(before, after)


func test_gen8() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_delete(after, 1)
	_add(after, 0)
	_move(after, 2, 0)
	_check(before, after)


func test_gen9() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_add(after, 2)
	_move(after, 0, 2)
	_check(before, after)


func test_gen10() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_move(after, 0, 1)
	_move(after, 1, 2)
	_add(after, 0)
	_check(before, after)


func test_gen11() -> void:
	var before := _init_with_size(4)
	var after: Array = before.duplicate()
	_move(after, 2, 0)
	_move(after, 2, 3)
	_check(before, after)


func test_gen12() -> void:
	var before := _init_with_size(4)
	var after: Array = before.duplicate()
	_move(after, 3, 0)
	_move(after, 2, 1)
	_check(before, after)


func test_gen13() -> void:
	var before := _init_with_size(4)
	var after: Array = before.duplicate()
	_move(after, 3, 2)
	_move(after, 0, 3)
	_check(before, after)


func test_gen14() -> void:
	var before := _init_with_size(4)
	var after: Array = before.duplicate()
	_move(after, 3, 2)
	_add(after, 4)
	_move(after, 0, 4)
	_check(before, after)


func test_gen15() -> void:
	var before := _init_with_size(1)
	var after: Array = before.duplicate()
	_update(after, 0)
	_update(after, 0)
	_update(after, 0)
	_check(before, after)


func test_gen16() -> void:
	var before := _init_with_size(1)
	var after: Array = before.duplicate()
	_update(after, 0)
	_move(after, 0, 0)
	_move(after, 0, 0)
	_add(after, 0)
	_check(before, after)


func test_gen17() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_move(after, 1, 0)
	_add(after, 2)
	_update(after, 1)
	_add(after, 0)
	_check(before, after)


func test_gen18() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_update_with_payload(after, 0)
	_check(before, after)


func test_gen19() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_move(after, 1, 1)
	_delete(after, 2)
	_move(after, 0, 1)
	_add(after, 0)
	_update(after, 1)
	_add(after, 1)
	_update_with_payload(after, 2)
	_add(after, 1)
	_delete(after, 1)
	_update_with_payload(after, 3)
	_add(after, 2)
	_move(after, 2, 1)
	_add(after, 2)
	_delete(after, 2)
	_delete(after, 1)
	_check(before, after)


func test_one_item() -> void:
	var before := _init_with_size(1)
	var after: Array = before.duplicate()
	_check(before, after)


func test_empty() -> void:
	var before := _init_with_size(0)
	var after: Array = before.duplicate()
	_check(before, after)


func test_add1() -> void:
	var before := _init_with_size(1)
	var after: Array = before.duplicate()
	_add(after, 1)
	_check(before, after)


func test_move1() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_move(after, 0, 2)
	_check(before, after)


func test_update1() -> void:
	var before := _init_with_size(3)
	var after: Array = before.duplicate()
	_update(after, 2)
	_check(before, after)


func test_update2() -> void:
	var before := _init_with_size(2)
	var after: Array = before.duplicate()
	_add(after, 1)
	_update(after, 1)
	_update(after, 2)
	_check(before, after)


func test_disable_move_detection() -> void:
	var before := _init_with_size(5)
	var after: Array = before.duplicate()
	_move(after, 0, 4)
	var callback := TestCallback.new()
	callback.old_list = before
	callback.new_list = after
	var result := DiffUtil.calculate_diff(callback, false)
	var applied: Array = before.duplicate()
	var id_state := { "next": _id_counter }
	var applier := ApplyingCallback.new(applied, id_state)
	result.dispatch_updates_to(applier)
	assert_that(applier.violations).is_empty()
	assert_that(applied.size()).is_equal(5)
	assert_that(applied[4].new_item).is_true()
	assert_that(applied.has(before[0])).is_false()


func test_duplicate() -> void:
	_id_counter = 0
	var before: Array = [_make_item(false), _make_item(false)]
	var after: Array = [before[0], before[1], _make_item(true), before[1]]
	_check(before, after)


func test_random_fuzz() -> void:
	# Port of the (commented-out) testRandom with a fixed seed for reproducibility.
	_rand_state = 123456789
	for round in 100:
		var before := _init_with_size(_next_rand() % 8)
		var after: Array = before.duplicate()
		var operation_count := 2 + _next_rand() % 40
		for i in operation_count:
			match _next_rand() % 6:
				0:
					_add(after, _next_rand() % (after.size() + 1))
				1:
					if not after.is_empty():
						_delete(after, _next_rand() % after.size())
				2:
					if not after.is_empty():
						_move(after, _next_rand() % after.size(), _next_rand() % after.size())
				3:
					if not after.is_empty():
						_update(after, _next_rand() % after.size())
				4:
					if not after.is_empty():
						_update_with_payload(after, _next_rand() % after.size())
				5:
					if not after.is_empty():
						_duplicate(after, _next_rand() % after.size(), _next_rand() % after.size())
		_check(before, after)


var _rand_state := 0


func _next_rand() -> int:
	_rand_state = (_rand_state * 1664525 + 1013904223) & 0x7fffffff
	return _rand_state
