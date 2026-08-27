# Port of SortedListTest exercising the GDScript-facing SortedList API.

extends GdUnitTestSuite

const INVALID_POSITION := -1

var _list: SortedList
var _cb: TestCallback


class Item:
	var id: int
	var cmp_field: int
	var data: int

	func _init(p_id: int, p_cmp_field: int = p_id, p_data: int = p_id) -> void:
		id = p_id
		cmp_field = p_cmp_field
		data = p_data

	func equals(other: Item) -> bool:
		return id == other.id and cmp_field == other.cmp_field and data == other.data


class TestCallback extends SortedListCallback:
	var additions: Array = []
	var removals: Array = []
	var moves: Array = []
	var updates: Array = []
	var payload_updates: Array = []
	var payload_mode: bool = false

	func _compare(o1, o2) -> int:
		if o1.cmp_field < o2.cmp_field:
			return -1
		if o1.cmp_field == o2.cmp_field:
			return 0
		return 1

	func _are_items_the_same(item1, item2) -> bool:
		return item1.id == item2.id

	func _are_contents_the_same(old_item, new_item) -> bool:
		return old_item.data == new_item.data

	func _get_change_payload(item1, item2) -> Variant:
		if payload_mode:
			return item2.data
		return null

	func _on_inserted(position: int, count: int) -> void:
		additions.append([position, count])

	func _on_removed(position: int, count: int) -> void:
		removals.append([position, count])

	func _on_moved(from_position: int, to_position: int) -> void:
		moves.append([from_position, to_position])

	func _on_changed(position: int, count: int) -> void:
		updates.append([position, count])

	func _on_changed_with_payload(position: int, count: int, payload: Variant) -> void:
		if payload_mode:
			payload_updates.append([position, count, payload])
		else:
			_on_changed(position, count)


func before_test() -> void:
	_cb = TestCallback.new()
	_list = SortedList.new()
	_list.set_callback(_cb)


func _make_items(count: int, step: int = 1) -> Array:
	var items: Array = []
	var id := 0
	for i in count:
		items.append(Item.new(id))
		id += step
	return items


func _contains(pairs: Array, pair: Array) -> bool:
	for p in pairs:
		if p[0] == pair[0] and p[1] == pair[1]:
			return true
	return false


func _assert_integrity(expected_size: int) -> void:
	assert_that(_list.size()).is_equal(expected_size)
	var range_start := 0
	for i in _list.size():
		var item: Item = _list.get(i)
		assert_that(_list.index_of(item)).is_equal(i)
		if i == 0:
			continue
		var cmp := _cb._compare(_list.get(i - 1), item)
		assert_that(cmp <= 0).is_true()
		if cmp == 0:
			for j in range(range_start, i):
				assert_that(_cb._are_items_the_same(_list.get(j), item)).is_false()
		else:
			range_start = i


func test_empty() -> void:
	assert_that(_list.size()).is_equal(0)


func test_add() -> void:
	var item := Item.new(1)
	assert_that(_list.add(item)).is_equal(0)
	assert_that(_list.size()).is_equal(1)
	assert_that(_contains(_cb.additions, [0, 1])).is_true()
	var item2 := Item.new(2)
	item2.cmp_field = item.cmp_field + 1
	assert_that(_list.add(item2)).is_equal(1)
	assert_that(_list.size()).is_equal(2)
	var item3 := Item.new(3)
	item3.cmp_field = item.cmp_field - 1
	_cb.additions.clear()
	assert_that(_list.add(item3)).is_equal(0)
	assert_that(_list.size()).is_equal(3)


func test_add_duplicate() -> void:
	var item := Item.new(1)
	var item2 := Item.new(item.id)
	assert_that(_list.add(item)).is_equal(0)
	assert_that(_list.add(item2)).is_equal(0)
	assert_that(_list.size()).is_equal(1)
	assert_that(_cb.additions.size()).is_equal(1)
	assert_that(_cb.updates.size()).is_equal(0)


func test_remove() -> void:
	var item := Item.new(1)
	assert_that(_list.remove(item)).is_false()
	assert_that(_list.add(item)).is_equal(0)
	assert_that(_list.remove(item)).is_true()
	assert_that(_cb.removals.size()).is_equal(1)
	assert_that(_contains(_cb.removals, [0, 1])).is_true()
	assert_that(_list.size()).is_equal(0)


func clear_test() -> void:
	_list.add(Item.new(1))
	_list.add(Item.new(2))
	assert_that(_list.size()).is_equal(2)
	_list.clear()
	assert_that(_list.size()).is_equal(0)


func test_batch() -> void:
	_list.begin_batched_updates()
	for i in 5:
		_list.add(Item.new(i))
	assert_that(_cb.additions.size()).is_equal(0)
	_list.end_batched_updates()
	assert_that(_contains(_cb.additions, [0, 5])).is_true()


func test_add_all_merge() -> void:
	_list.add_all(_make_items(5, 2))
	_assert_integrity(5)
	assert_that(_cb.additions.size()).is_equal(1)
	assert_that(_contains(_cb.additions, [0, 5])).is_true()

	# Merge 0..4 into the existing evens 0,2,4,6,8 -> 1,3 are inserted, 6,8 stay.
	_list.add_all(_make_items(5))
	_assert_integrity(7)
	assert_that(_cb.removals.size()).is_equal(0)
	assert_that(_cb.moves.size()).is_equal(0)


func test_add_all_updates() -> void:
	var even := _make_items(5, 2)
	for item in even:
		item.data = 1
	_list.add_all(even)
	assert_that(_list.size()).is_equal(5)
	assert_that(_cb.updates.size()).is_equal(0)

	var new_even := _make_items(5, 2)
	for item in new_even:
		item.data = 2
	_list.add_all(new_even)
	assert_that(_list.size()).is_equal(5)
	assert_that(_cb.updates.size()).is_equal(1)
	assert_that(_contains(_cb.updates, [0, 5])).is_true()
	for i in 5:
		assert_that(_list.get(i).data).is_equal(2)


func test_payload_on_add_existing() -> void:
	_list.add_all([Item.new(1), Item.new(2), Item.new(3)])
	_cb.payload_mode = true
	var two := Item.new(2)
	two.data = 1337
	_list.add(two)
	assert_that(_cb.payload_updates.size()).is_equal(1)
	assert_that(_cb.payload_updates[0][0]).is_equal(1)
	assert_that(_cb.payload_updates[0][1]).is_equal(1)
	assert_that(_cb.payload_updates[0][2]).is_equal(1337)


func test_update_item_calls_change_with_payload() -> void:
	_list.add_all([Item.new(1), Item.new(2), Item.new(3)])
	_cb.payload_mode = true
	var two := Item.new(2)
	two.data = 1337
	_list.update_item_at(1, two)
	assert_that(_cb.payload_updates.size()).is_equal(1)
	assert_that(_cb.payload_updates[0][0]).is_equal(1)
	assert_that(_cb.payload_updates[0][2]).is_equal(1337)
	assert_that(_list.get(1).data).is_equal(1337)


func test_replace_all_totally_equivalent() -> void:
	_list.add_all(_make_items(3))
	_list.replace_all(_make_items(3))
	assert_that(_list.size()).is_equal(3)
	for i in 3:
		assert_that(_list.get(i).id).is_equal(i)


func test_replace_all_sorted_and_deduped() -> void:
	_list.replace_all([Item.new(2), Item.new(1), Item.new(1)])
	assert_that(_list.size()).is_equal(2)
	assert_that(_list.get(0).id).is_equal(1)
	assert_that(_list.get(1).id).is_equal(2)


func test_index_of_and_get() -> void:
	_list.add_all([Item.new(3), Item.new(1), Item.new(2)])
	assert_that(_list.get(0).id).is_equal(1)
	assert_that(_list.get(1).id).is_equal(2)
	assert_that(_list.get(2).id).is_equal(3)
	assert_that(_list.index_of(Item.new(2))).is_equal(1)
	assert_that(_list.index_of(Item.new(9))).is_equal(INVALID_POSITION)


func test_update_item_at_reorders() -> void:
	_list.add_all([Item.new(1), Item.new(2), Item.new(3)])
	# Move item 2 (cmp 2) to the end by raising its sort field.
	var replacement := Item.new(2, 9, 9)
	_list.update_item_at(1, replacement)
	assert_that(_list.get(0).id).is_equal(1)
	assert_that(_list.get(1).id).is_equal(3)
	assert_that(_list.get(2).id).is_equal(2)
	assert_that(_cb.moves.size() >= 1).is_true()


func test_random() -> void:
	var copy: Array = []
	var rng := RandomNumberGenerator.new()
	rng.seed = 12345
	var id := 1
	for i in 500:
		var op := rng.randi_range(0, 2)
		match op:
			0:
				var item := Item.new(id)
				id += 1
				copy.append(item)
				_list.add(item)
			1:
				if not copy.is_empty():
					var index := rng.randi_range(0, _list.size() - 1)
					var item: Item = _list.get(index)
					copy.erase(item)
					assert_that(_list.remove(item)).is_true()
			2:
				if not copy.is_empty():
					var index := rng.randi_range(0, _list.size() - 1)
					var item: Item = _list.get(index)
					var new_item := Item.new(item.id, item.cmp_field, rng.randi_range(0, 1000))
					while new_item.data == item.data:
						new_item.data = rng.randi_range(0, 1000)
					_list.add(new_item)
					copy.erase(item)
					copy.append(new_item)
		assert_that(copy.size()).is_equal(_list.size())
		var last_cmp := -2147483648
		for idx in copy.size():
			assert_that(_list.index_of(copy[idx]) != INVALID_POSITION).is_true()
			assert_that(_list.get(idx).cmp_field >= last_cmp).is_true()
			last_cmp = _list.get(idx).cmp_field
			assert_that(copy.has(_list.get(idx))).is_true()
