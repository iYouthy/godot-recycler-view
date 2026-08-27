// Port of androidx SortedListTest.java exercising the pure C++ SortedListCore<T>.

#include "doctest.h"

#include "sorted_list.h"

using namespace godot;

namespace {

struct TestItem {
	int id;
	int cmp_field;
	int data;

	TestItem() :
			id(0), cmp_field(0), data(0) {}

	TestItem(int p_all_fields) :
			id(p_all_fields), cmp_field(p_all_fields), data(p_all_fields) {}

	TestItem(int p_id, int p_cmp_field, int p_data) :
			id(p_id), cmp_field(p_cmp_field), data(p_data) {}

	bool operator==(const TestItem &p_other) const {
		return id == p_other.id && cmp_field == p_other.cmp_field && data == p_other.data;
	}
};

int next_rand(unsigned int &p_state) {
	p_state = p_state * 1664525u + 1013904223u;
	return (int)(p_state >> 16) & 0x7fffffff;
}

int next_int(unsigned int &p_state, int p_n) {
	if (p_n == 0) {
		return 0;
	}
	return next_rand(p_state) % p_n;
}

struct Harness {
	enum Type {
		TYPE_ADD,
		TYPE_REMOVE,
		TYPE_MOVE,
		TYPE_CHANGE,
	};

	struct Event {
		Type type;
		int val1;
		int val2;

		bool operator==(const Event &p_other) const {
			return type == p_other.type && val1 == p_other.val1 && val2 == p_other.val2;
		}
	};

	struct Pair {
		int first;
		int second;

		bool operator==(const Pair &p_other) const {
			return first == p_other.first && second == p_other.second;
		}
	};

	struct PayloadChange {
		int position;
		int count;
		int payload;

		bool operator==(const PayloadChange &p_other) const {
			return position == p_other.position && count == p_other.count && payload == p_other.payload;
		}
	};

	// Verifies the list state at a moment during a callback.
	struct StateCheck {
		SortedListCore<TestItem> *list;
		Vector<TestItem> expected;

		void run() const {
			REQUIRE(list->size() == expected.size());
			for (int i = 0; i < expected.size(); i++) {
				REQUIRE(list->get(i) == expected[i]);
				REQUIRE(list->index_of(expected[i]) == i);
			}
		}
	};

	struct Callback : SortedListCoreCallback<TestItem> {
		Harness *h;

		explicit Callback(Harness *p_h) :
				h(p_h) {}

		int compare(const TestItem &p_o1, const TestItem &p_o2) override {
			return p_o1.cmp_field < p_o2.cmp_field ? -1 : (p_o1.cmp_field == p_o2.cmp_field ? 0 : 1);
		}

		bool are_items_the_same(const TestItem &p_item1, const TestItem &p_item2) override {
			return p_item1.id == p_item2.id;
		}

		bool are_contents_the_same(const TestItem &p_old_item, const TestItem &p_new_item) override {
			return p_old_item.data == p_new_item.data;
		}

		const void *get_change_payload(const TestItem &p_item1, const TestItem &p_item2) override {
			(void)p_item1;
			return h->payload_changes ? (const void *)(intptr_t)p_item2.data : nullptr;
		}

		void on_inserted(int p_position, int p_count) override {
			h->events.push_back(Event({ TYPE_ADD, p_position, p_count }));
			h->additions.push_back(Pair({ p_position, p_count }));
			h->run_inserted_hook(p_position, p_count);
			h->poll_and_run();
		}

		void on_removed(int p_position, int p_count) override {
			h->events.push_back(Event({ TYPE_REMOVE, p_position, p_count }));
			h->removals.push_back(Pair({ p_position, p_count }));
			h->poll_and_run();
		}

		void on_moved(int p_from_position, int p_to_position) override {
			h->events.push_back(Event({ TYPE_MOVE, p_from_position, p_to_position }));
			h->moves.push_back(Pair({ p_from_position, p_to_position }));
		}

		void on_changed(int p_position, int p_count, const void *p_payload) override {
			if (h->payload_changes) {
				h->payload_updates.push_back(PayloadChange({ p_position, p_count, (int)(intptr_t)p_payload }));
			} else {
				h->events.push_back(Event({ TYPE_CHANGE, p_position, p_count }));
				h->updates.push_back(Pair({ p_position, p_count }));
				h->run_changed_hook(p_position, p_count);
				h->poll_and_run();
			}
		}
	};

	Callback callback;
	SortedListCore<TestItem> list;

	Vector<Pair> additions;
	Vector<Pair> removals;
	Vector<Pair> moves;
	Vector<Pair> updates;
	Vector<Event> events;
	Vector<PayloadChange> payload_updates;
	bool payload_changes = false;

	// Reentrancy test hooks (mode 0 = none).
	int hook_mode = 0;
	Vector<TestItem> *hook_items = nullptr;
	int *hook_counter = nullptr;
	Vector<StateCheck> callback_runnables;

	Harness() :
			callback(this), list(&callback) {}

	void poll_and_run() {
		if (!callback_runnables.is_empty()) {
			StateCheck check = callback_runnables[0];
			callback_runnables.remove_at(0);
			check.run();
		}
	}

	void run_inserted_hook(int p_position, int p_count) {
		if (hook_mode == 1) {
			for (int i = 0; i < p_count; i++) {
				REQUIRE(list.get(i) == (*hook_items)[i]);
				REQUIRE(list.index_of((*hook_items)[i]) == i);
				(*hook_counter)++;
			}
		} else if (hook_mode == 2) {
			(void)p_position;
			(void)p_count;
			// All mutations must be rejected while a callback is running.
			REQUIRE(list.add(TestItem(1)) == SortedListCore<TestItem>::INVALID_POSITION);
			Vector<TestItem> more;
			more.push_back(TestItem(0));
			list.add_all(more); // no-op
			REQUIRE_FALSE(list.remove(TestItem(0)));
			REQUIRE(list.remove_item_at(0).id == 0); // returns default on rejection
			list.update_item_at(0, TestItem(0)); // no-op
			list.recalculate_position_of_item_at(0); // no-op
			list.clear(); // no-op
		}
	}

	void run_changed_hook(int p_position, int p_count) {
		(void)p_position;
		(void)p_count;
	}

	bool contains_pair(const Vector<Pair> &p_pairs, const Pair &p_pair) const {
		for (const Pair &p : p_pairs) {
			if (p == p_pair) {
				return true;
			}
		}
		return false;
	}

	int size() const { return list.size(); }

	int insert(const TestItem &p_item) { return list.add(p_item); }

	bool remove(const TestItem &p_item) { return list.remove(p_item); }

	// ------------------------------------------------------------------
	// Helpers (port of createItems / createItemsFromInts).

	Vector<TestItem> create_items(int p_id_from, int p_id_to, int p_id_step) {
		const int count = (p_id_to - p_id_from) / p_id_step + 1;
		Vector<TestItem> items;
		items.resize(count);
		int id = p_id_from;
		for (int i = 0; i < count; i++) {
			items.write[i] = TestItem(id);
			id += p_id_step;
		}
		return items;
	}

	Vector<TestItem> create_items_from_ints(const Vector<int> &p_ints) {
		Vector<TestItem> items;
		items.resize(p_ints.size());
		for (int i = 0; i < p_ints.size(); i++) {
			items.write[i] = TestItem(p_ints[i]);
		}
		return items;
	}

	// ------------------------------------------------------------------
	// Integrity checks.

	void assert_integrity(int p_expected_size, const char *p_context) {
		REQUIRE_MESSAGE(list.size() == p_expected_size, p_context);
		int range_start = 0;
		for (int i = 0; i < list.size(); i++) {
			TestItem item = list.get(i);
			REQUIRE_MESSAGE(list.index_of(item) == i, p_context);
			if (i == 0) {
				continue;
			}
			const int compare = callback.compare(list.get(i - 1), item);
			REQUIRE_MESSAGE(compare <= 0, p_context);
			if (compare == 0) {
				for (int j = range_start; j < i; j++) {
					REQUIRE_MESSAGE(!callback.are_items_the_same(list.get(j), item), p_context);
				}
			} else {
				range_start = i;
			}
		}
	}

	void assert_sequential_order() {
		for (int i = 0; i < size(); i++) {
			REQUIRE(list.get(i).cmp_field == i);
		}
	}

	bool sorted_list_equals(const SortedListCore<TestItem> &p_list, const Vector<TestItem> &p_array) {
		if (p_list.size() != p_array.size()) {
			return false;
		}
		for (int i = 0; i < p_list.size(); i++) {
			if (!(p_list.get(i) == p_array[i])) {
				return false;
			}
		}
		return true;
	}
};

} // namespace

// ---------------------------------------------------------------------------
// Core tests.

TEST_CASE("test_empty") {
	Harness h;
	REQUIRE(h.size() == 0);
}

TEST_CASE("test_add") {
	Harness h;
	TestItem item(1);
	REQUIRE(h.insert(item) == 0);
	REQUIRE(h.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 1 })));
	TestItem item2(2);
	item2.cmp_field = item.cmp_field + 1;
	REQUIRE(h.insert(item2) == 1);
	REQUIRE(h.size() == 2);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 1, 1 })));
	TestItem item3(3);
	item3.cmp_field = item.cmp_field - 1;
	h.additions.clear();
	REQUIRE(h.insert(item3) == 0);
	REQUIRE(h.size() == 3);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 1 })));
}

TEST_CASE("test_add_duplicate") {
	Harness h;
	TestItem item(1);
	TestItem item2(item.id);
	REQUIRE(h.insert(item) == 0);
	REQUIRE(h.insert(item2) == 0);
	REQUIRE(h.size() == 1);
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.updates.size() == 0);
}

TEST_CASE("test_remove") {
	Harness h;
	TestItem item(1);
	REQUIRE_FALSE(h.remove(item));
	REQUIRE(h.removals.size() == 0);
	REQUIRE(h.insert(item) == 0);
	REQUIRE(h.remove(item));
	REQUIRE(h.removals.size() == 1);
	REQUIRE(h.contains_pair(h.removals, Harness::Pair({ 0, 1 })));
	REQUIRE(h.size() == 0);
	REQUIRE_FALSE(h.remove(item));
	REQUIRE(h.removals.size() == 1);
}

TEST_CASE("test_remove2") {
	Harness h;
	TestItem item(1);
	TestItem item2(2, 1, 1);
	REQUIRE(h.insert(item) == 0);
	REQUIRE_FALSE(h.remove(item2));
	REQUIRE(h.removals.size() == 0);
}

TEST_CASE("clear_test") {
	Harness h;
	REQUIRE(h.insert(TestItem(1)) == 0);
	REQUIRE(h.insert(TestItem(2)) == 1);
	REQUIRE(h.list.size() == 2);
	h.list.clear();
	REQUIRE(h.list.size() == 0);
	REQUIRE(h.insert(TestItem(3)) == 0);
	REQUIRE(h.list.size() == 1);
}

TEST_CASE("test_batch") {
	Harness h;
	h.list.begin_batched_updates();
	for (int i = 0; i < 5; i++) {
		h.list.add(TestItem(i));
	}
	REQUIRE(h.additions.size() == 0);
	h.list.end_batched_updates();
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 5 })));
}

// ---------------------------------------------------------------------------
// addAll tests.

TEST_CASE("test_add_all_merge") {
	Harness h;
	h.list.add_all(Vector<TestItem>());
	h.assert_integrity(0, "addAll, empty list, empty input");
	REQUIRE(h.additions.size() == 0);

	h.list.add_all(h.create_items(0, 8, 2));
	h.assert_integrity(5, "addAll, empty list, non-empty input");
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 5 })));

	h.list.add_all(Vector<TestItem>());
	h.assert_integrity(5, "addAll, non-empty list, empty input");
	REQUIRE(h.additions.size() == 1);

	h.list.add_all(h.create_items(10, 18, 2));
	h.assert_integrity(10, "addAll, sequential input");
	REQUIRE(h.additions.size() == 2);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 5, 5 })));

	h.list.add_all(h.create_items(28, 20, -2));
	h.assert_integrity(15, "addAll, reversed input");
	REQUIRE(h.additions.size() == 3);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 10, 5 })));

	h.list.add_all(h.create_items(1, 19, 2));
	h.assert_integrity(25, "addAll, merging in the middle");
	REQUIRE(h.additions.size() == 13);
	for (int i = 1; i <= 19; i += 2) {
		REQUIRE(h.contains_pair(h.additions, Harness::Pair({ i, 1 })));
	}

	h.list.add_all(h.create_items(21, 39, 2));
	h.assert_integrity(35, "addAll, merging at the end");
	REQUIRE(h.additions.size() == 18);
	for (int i = 21; i <= 27; i += 2) {
		REQUIRE(h.contains_pair(h.additions, Harness::Pair({ i, 1 })));
	}
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 29, 6 })));

	h.list.add_all(h.create_items(30, 38, 2));
	h.assert_integrity(40, "addAll, merging more");
	REQUIRE(h.additions.size() == 23);
	for (int i = 30; i <= 38; i += 2) {
		REQUIRE(h.contains_pair(h.additions, Harness::Pair({ i, 1 })));
	}

	REQUIRE(h.moves.size() == 0);
	REQUIRE(h.updates.size() == 0);
	REQUIRE(h.removals.size() == 0);
	h.assert_sequential_order();
}

TEST_CASE("test_add_all_updates") {
	Harness h;
	Vector<TestItem> even_items = h.create_items(0, 8, 2);
	for (int i = 0; i < even_items.size(); i++) {
		even_items.write[i].data = 1;
	}
	h.list.add_all(even_items);
	REQUIRE(h.size() == 5);
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 5 })));
	REQUIRE(h.updates.size() == 0);

	Vector<TestItem> same_even_items = h.create_items(0, 8, 2);
	for (int i = 0; i < same_even_items.size(); i++) {
		same_even_items.write[i].data = 1;
	}
	h.list.add_all(same_even_items);
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.updates.size() == 0);

	Vector<TestItem> new_even_items = h.create_items(0, 8, 2);
	for (int i = 0; i < new_even_items.size(); i++) {
		new_even_items.write[i].data = 2;
	}
	h.list.add_all(new_even_items);
	REQUIRE(h.size() == 5);
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.updates.size() == 1);
	REQUIRE(h.contains_pair(h.updates, Harness::Pair({ 0, 5 })));
	for (int i = 0; i < 5; i++) {
		REQUIRE(h.list.get(i).data == 2);
	}

	Vector<TestItem> sequential_items = h.create_items(0, 9, 1);
	for (int i = 0; i < sequential_items.size(); i++) {
		sequential_items.write[i].data = 3;
	}
	h.list.add_all(sequential_items);

	REQUIRE(h.additions.size() == 6);
	for (int i = 0; i < 5; i++) {
		REQUIRE(h.contains_pair(h.additions, Harness::Pair({ i * 2 + 1, 1 })));
	}
	REQUIRE(h.updates.size() == 6);
	for (int i = 0; i < 5; i++) {
		REQUIRE(h.contains_pair(h.updates, Harness::Pair({ i * 2, 1 })));
	}
	REQUIRE(h.size() == 10);
	for (int i = 0; i < 10; i++) {
		REQUIRE(h.list.get(i).data == 3);
	}
	REQUIRE(h.moves.size() == 0);
	REQUIRE(h.removals.size() == 0);
	h.assert_sequential_order();
}

TEST_CASE("test_add_all_with_duplicates") {
	Harness h;
	const int max_cmp_field = 5;
	const int ids_per_cmp_field = 10;
	const int max_unique_id = max_cmp_field * ids_per_cmp_field;
	const int max_generation = 5;

	Vector<TestItem> items;
	items.resize(max_unique_id * max_generation);
	int index = 0;
	for (int generation = 0; generation < max_generation; generation++) {
		int unique_id = 0;
		for (int cmp_field = 0; cmp_field < max_cmp_field; cmp_field++) {
			for (int id = 0; id < ids_per_cmp_field; id++) {
				items.write[index++] = TestItem(unique_id++, cmp_field, generation);
			}
		}
	}

	h.list.add_all(items);
	h.assert_integrity(max_unique_id, "addAll with duplicates");
	for (int i = 0; i < h.size(); i++) {
		REQUIRE(h.list.get(i).data == max_generation - 1);
	}
}

TEST_CASE("test_add_all_fast") {
	Harness h;
	h.list.add_all(Vector<TestItem>());
	h.assert_integrity(0, "addAll, empty list, with empty input");
	REQUIRE(h.additions.size() == 0);

	h.list.add_all(h.create_items(0, 9, 1));
	h.assert_integrity(10, "addAll, empty list, non-empty input");
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 10 })));

	h.list.add_all(Vector<TestItem>());
	REQUIRE(h.additions.size() == 1);
	h.assert_integrity(10, "addAll, non-empty list, empty input");

	h.list.add_all(h.create_items(10, 19, 1));
	REQUIRE(h.additions.size() == 2);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 10, 10 })));
	h.assert_integrity(20, "addAll, non-empty list, non-empty input");
}

TEST_CASE("test_add_all_collection") {
	Harness h;
	Vector<TestItem> items;
	for (int i = 0; i < 5; i++) {
		items.push_back(TestItem(i));
	}
	h.list.add_all(items);
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, (int)items.size() })));
	h.assert_integrity((int)items.size(), "addAll on collection");
}

TEST_CASE("test_add_all_stable_sort") {
	Harness h;
	int id = 0;
	TestItem item(id++, 0, 0);
	h.list.add(item);

	Vector<TestItem> items;
	items.resize(3);
	for (int i = 0; i < 3; i++) {
		items.write[i] = TestItem(id++, item.cmp_field, 0);
		REQUIRE(h.callback.compare(item, items[i]) == 0);
	}

	h.list.add_all(items);
	REQUIRE(h.size() == 1 + items.size());
	for (int i = 0; i < h.size(); i++) {
		REQUIRE(h.list.get(i).id == i);
	}
}

TEST_CASE("test_add_all_outside_batched_updates") {
	Harness h;
	h.list.add(TestItem(1));
	REQUIRE(h.additions.size() == 1);
	h.list.add(TestItem(2));
	REQUIRE(h.additions.size() == 2);
	Vector<TestItem> items;
	items.push_back(TestItem(3));
	items.push_back(TestItem(4));
	h.list.add_all(items);
	REQUIRE(h.additions.size() == 3);
	h.list.add(TestItem(5));
	REQUIRE(h.additions.size() == 4);
	h.list.add(TestItem(6));
	REQUIRE(h.additions.size() == 5);
}

TEST_CASE("test_add_all_inside_batched_updates") {
	Harness h;
	h.list.begin_batched_updates();
	h.list.add(TestItem(1));
	REQUIRE(h.additions.size() == 0);
	h.list.add(TestItem(2));
	REQUIRE(h.additions.size() == 0);
	Vector<TestItem> items;
	items.push_back(TestItem(3));
	items.push_back(TestItem(4));
	h.list.add_all(items);
	REQUIRE(h.additions.size() == 0);
	h.list.add(TestItem(5));
	REQUIRE(h.additions.size() == 0);
	h.list.add(TestItem(6));
	REQUIRE(h.additions.size() == 0);
	h.list.end_batched_updates();
	REQUIRE(h.additions.size() == 1);
	REQUIRE(h.contains_pair(h.additions, Harness::Pair({ 0, 6 })));
}

// ---------------------------------------------------------------------------
// Payload tests.

TEST_CASE("test_add_existing_item_calls_change_with_payload") {
	Harness h;
	Vector<TestItem> items;
	items.push_back(TestItem(1));
	items.push_back(TestItem(2));
	items.push_back(TestItem(3));
	h.list.add_all(items);
	h.payload_changes = true;

	TestItem two_update(2);
	two_update.data = 1337;
	h.list.add(two_update);
	REQUIRE(h.payload_updates.size() == 1);
	REQUIRE(h.payload_updates[0].position == 1);
	REQUIRE(h.payload_updates[0].count == 1);
	REQUIRE(h.payload_updates[0].payload == 1337);
	REQUIRE(h.size() == 3);
}

TEST_CASE("test_update_item_calls_change_with_payload") {
	Harness h;
	Vector<TestItem> items;
	items.push_back(TestItem(1));
	items.push_back(TestItem(2));
	items.push_back(TestItem(3));
	h.list.add_all(items);
	h.payload_changes = true;

	TestItem two_update(2);
	two_update.data = 1337;
	h.list.update_item_at(1, two_update);
	REQUIRE(h.payload_updates.size() == 1);
	REQUIRE(h.payload_updates[0].position == 1);
	REQUIRE(h.payload_updates[0].count == 1);
	REQUIRE(h.payload_updates[0].payload == 1337);
	REQUIRE(h.size() == 3);
	REQUIRE(h.list.get(1).data == 1337);
}

TEST_CASE("test_add_multiple_existing_item_calls_change_with_payload") {
	Harness h;
	Vector<TestItem> items;
	items.push_back(TestItem(1));
	items.push_back(TestItem(2));
	items.push_back(TestItem(3));
	h.list.add_all(items);
	h.payload_changes = true;

	TestItem two_update(2);
	two_update.data = 222;
	TestItem three_update(3);
	three_update.data = 333;
	Vector<TestItem> updates;
	updates.push_back(two_update);
	updates.push_back(three_update);
	h.list.add_all(updates);

	REQUIRE(h.payload_updates.size() == 2);
	REQUIRE(h.payload_updates[0].position == 1);
	REQUIRE(h.payload_updates[0].count == 1);
	REQUIRE(h.payload_updates[0].payload == 222);
	REQUIRE(h.payload_updates[1].position == 2);
	REQUIRE(h.payload_updates[1].count == 1);
	REQUIRE(h.payload_updates[1].payload == 333);
	REQUIRE(h.size() == 3);
}

// ---------------------------------------------------------------------------
// replaceAll tests.

TEST_CASE("replace_all_totally_equivalent_data_works_correctly") {
	Harness h;
	Vector<TestItem> items1 = h.create_items_from_ints(Vector<int>({ 1, 2, 3 }));
	Vector<TestItem> items2 = h.create_items_from_ints(Vector<int>({ 1, 2, 3 }));
	h.list.add_all(items1);
	h.events.clear();
	h.list.replace_all(items2);
	REQUIRE(h.events.is_empty());
	REQUIRE(h.sorted_list_equals(h.list, items2));
}

TEST_CASE("replace_all_removals_and_adds1_works_correctly") {
	Harness h;
	Vector<TestItem> items1 = h.create_items_from_ints(Vector<int>({ 1, 3, 5 }));
	Vector<TestItem> items2 = h.create_items_from_ints(Vector<int>({ 2, 4 }));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, h.create_items_from_ints(Vector<int>({ 2, 3, 5 })) }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, h.create_items_from_ints(Vector<int>({ 2, 5 })) }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, h.create_items_from_ints(Vector<int>({ 2, 4, 5 })) }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.replace_all(items2);

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_REMOVE, 0, 1 }));
	REQUIRE(h.events[1] == Harness::Event({ Harness::TYPE_ADD, 0, 1 }));
	REQUIRE(h.events[2] == Harness::Event({ Harness::TYPE_REMOVE, 1, 1 }));
	REQUIRE(h.events[3] == Harness::Event({ Harness::TYPE_ADD, 1, 1 }));
	REQUIRE(h.events[4] == Harness::Event({ Harness::TYPE_REMOVE, 2, 1 }));
	REQUIRE(h.events.size() == 5);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_removals_and_adds5_works_correctly") {
	Harness h;
	Vector<TestItem> items1 = h.create_items_from_ints(Vector<int>({ 1, 2, 3 }));
	Vector<TestItem> items2 = h.create_items_from_ints(Vector<int>({ 3, 4, 5 }));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.replace_all(items2);

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_REMOVE, 0, 2 }));
	REQUIRE(h.events[1] == Harness::Event({ Harness::TYPE_ADD, 1, 2 }));
	REQUIRE(h.events.size() == 2);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_move1_works_correctly") {
	Harness h;
	Vector<TestItem> items1 = h.create_items_from_ints(Vector<int>({ 1, 2, 3 }));
	Vector<TestItem> items2;
	items2.push_back(TestItem(2));
	items2.push_back(TestItem(3));
	items2.push_back(TestItem(1, 4, 1));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.replace_all(items2);

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_REMOVE, 0, 1 }));
	REQUIRE(h.events[1] == Harness::Event({ Harness::TYPE_ADD, 2, 1 }));
	REQUIRE(h.events.size() == 2);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_order_same_item_different_works_correctly") {
	Harness h;
	Vector<TestItem> items1;
	items1.push_back(TestItem(1));
	items1.push_back(TestItem(2, 3, 2));
	items1.push_back(TestItem(5));
	Vector<TestItem> items2;
	items2.push_back(TestItem(1));
	items2.push_back(TestItem(4, 3, 4));
	items2.push_back(TestItem(5));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.replace_all(items2);

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_REMOVE, 1, 1 }));
	REQUIRE(h.events[1] == Harness::Event({ Harness::TYPE_ADD, 1, 1 }));
	REQUIRE(h.events.size() == 2);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_order_same_item_same_contents_different_works_correctly") {
	Harness h;
	Vector<TestItem> items1;
	items1.push_back(TestItem(1));
	items1.push_back(TestItem(3, 3, 2));
	items1.push_back(TestItem(5));
	Vector<TestItem> items2;
	items2.push_back(TestItem(1));
	items2.push_back(TestItem(3, 3, 4));
	items2.push_back(TestItem(5));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.replace_all(items2);

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_CHANGE, 1, 1 }));
	REQUIRE(h.events.size() == 1);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_new_items_are_identical_result_is_deduped") {
	Harness h;
	Vector<TestItem> items = h.create_items_from_ints(Vector<int>({ 1, 1 }));
	h.list.replace_all(items);
	REQUIRE(h.list.get(0) == TestItem(1));
	REQUIRE(h.list.size() == 1);
}

TEST_CASE("replace_all_new_items_unsorted_result_is_sorted") {
	Harness h;
	Vector<TestItem> items = h.create_items_from_ints(Vector<int>({ 2, 1 }));
	h.list.replace_all(items);
	REQUIRE(h.list.get(0) == TestItem(1));
	REQUIRE(h.list.get(1) == TestItem(2));
	REQUIRE(h.list.size() == 2);
}

TEST_CASE("replace_all_called_after_begin_batched_updates_works_correctly") {
	Harness h;
	Vector<TestItem> items1 = h.create_items_from_ints(Vector<int>({ 1, 2, 3 }));
	Vector<TestItem> items2 = h.create_items_from_ints(Vector<int>({ 4, 5, 6 }));
	h.list.add_all(items1);
	h.events.clear();

	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));
	h.callback_runnables.push_back(Harness::StateCheck({ &h.list, items2 }));

	h.list.begin_batched_updates();
	h.list.replace_all(items2);
	h.list.end_batched_updates();

	REQUIRE(h.events[0] == Harness::Event({ Harness::TYPE_REMOVE, 0, 3 }));
	REQUIRE(h.events[1] == Harness::Event({ Harness::TYPE_ADD, 0, 3 }));
	REQUIRE(h.events.size() == 2);
	REQUIRE(h.sorted_list_equals(h.list, items2));
	REQUIRE(h.callback_runnables.is_empty());
}

TEST_CASE("replace_all_calls_change_with_payload") {
	Harness h;
	Vector<TestItem> items;
	items.push_back(TestItem(1));
	items.push_back(TestItem(2));
	items.push_back(TestItem(3));
	h.list.add_all(items);
	h.payload_changes = true;

	TestItem two_update(2);
	two_update.data = 222;
	TestItem three_update(3);
	three_update.data = 333;
	Vector<TestItem> updates;
	updates.push_back(two_update);
	updates.push_back(three_update);
	h.list.replace_all(updates);

	REQUIRE(h.payload_updates.size() == 2);
	REQUIRE(h.payload_updates[0].position == 0);
	REQUIRE(h.payload_updates[0].count == 1);
	REQUIRE(h.payload_updates[0].payload == 222);
	REQUIRE(h.payload_updates[1].position == 1);
	REQUIRE(h.payload_updates[1].count == 1);
	REQUIRE(h.payload_updates[1].payload == 333);
}

// ---------------------------------------------------------------------------
// Reentrancy / error tests.

TEST_CASE("test_valid_methods_during_on_inserted_callback_from_empty_list") {
	Harness h;
	Vector<TestItem> items;
	items.push_back(TestItem(0));
	items.push_back(TestItem(1));
	items.push_back(TestItem(2));

	int counter = 0;
	h.hook_mode = 1;
	h.hook_items = &items;
	h.hook_counter = &counter;

	h.list.add(items[0]);
	h.list.clear();
	h.list.add_all(items);
	REQUIRE(counter == 4);
}

TEST_CASE("test_modification_from_callback_throws") {
	Harness h;
	Vector<TestItem> items = h.create_items(1, 5, 2);
	for (int i = 0; i < items.size(); i++) {
		items.write[i].data = 1;
	}
	h.list.add_all(items);

	h.hook_mode = 2;

	items = h.create_items(1, 5, 1);
	for (int i = 0; i < items.size(); i++) {
		items.write[i].data = 2;
	}
	h.list.add_all(items);
	h.assert_integrity(5, "Modification from callback");
}

// ---------------------------------------------------------------------------
// Random fuzz (deterministic seed).

TEST_CASE("test_random") {
	Harness h;
	Vector<TestItem> copy;
	unsigned int state = 123456789u;
	int id = 1;
	for (int i = 0; i < 10000; i++) {
		switch (next_int(state, 3)) {
			case 0: { // ADD
				TestItem item(id++);
				copy.push_back(item);
				h.insert(item);
				break;
			}
			case 1: // REMOVE
				if (!copy.is_empty()) {
					int index = next_int(state, h.list.size());
					TestItem item = h.list.get(index);
					copy.remove_at(copy.find(item));
					REQUIRE(h.list.remove(item));
				}
				break;
			case 2: { // UPDATE
				if (!copy.is_empty()) {
					int index = next_int(state, h.list.size());
					TestItem item = h.list.get(index);
					TestItem new_item(item.id, item.cmp_field, next_int(state, 1000));
					while (new_item.data == item.data) {
						new_item.data = next_int(state, 1000);
					}
					int item_index = h.list.add(new_item);
					copy.remove_at(copy.find(item));
					copy.push_back(new_item);
					REQUIRE(h.list.get(item_index) == new_item);
				}
				break;
			}
		}
		int last_cmp = -2147483647 - 1;
		REQUIRE(copy.size() == h.list.size());
		for (int index = 0; index < copy.size(); index++) {
			REQUIRE(h.list.index_of(copy[index]) != SortedListCore<TestItem>::INVALID_POSITION);
			REQUIRE(h.list.get(index).cmp_field >= last_cmp);
			last_cmp = h.list.get(index).cmp_field;
			REQUIRE(copy.has(h.list.get(index)));
		}
	}
}
