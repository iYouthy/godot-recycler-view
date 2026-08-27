// Port of androidx DiffUtilTest.kt exercising the pure algorithm layer
// (DiffAlgorithm / DiffResultData) through C++ interfaces.

#include "doctest.h"

#include "diff_algo.h"

using namespace godot;

namespace {

// ---------------------------------------------------------------------------
// Test data model (mirrors DiffUtilTest.kt's Item data class).

struct Item {
	int64_t id;
	bool new_item;
	bool changed;
	const void *payload;
	int64_t data;

	Item() :
			id(0), new_item(false), changed(false), payload(nullptr), data(0) {}

	Item(int64_t p_id, bool p_new_item, int64_t p_data) :
			id(p_id), new_item(p_new_item), changed(false), payload(nullptr), data(p_data) {}

	bool operator==(const Item &p_other) const {
		return id == p_other.id && new_item == p_other.new_item && changed == p_other.changed && payload == p_other.payload && data == p_other.data;
	}
};

int64_t g_id_counter = 0;
int64_t g_data_counter = 0;
int64_t g_payload_counter = 0;

// ---------------------------------------------------------------------------
// Callback + update application (mirrors ItemListCallback + applyUpdates).

struct TestCallback : DiffCallback {
	Vector<Item> *old_list;
	Vector<Item> *new_list;

	int get_old_list_size() override { return old_list->size(); }
	int get_new_list_size() override { return new_list->size(); }

	bool are_items_the_same(int p_old, int p_new) override {
		return (*old_list)[p_old].id == (*new_list)[p_new].id;
	}

	bool are_contents_the_same(int p_old, int p_new) override {
		REQUIRE((*old_list)[p_old].id == (*new_list)[p_new].id);
		return (*old_list)[p_old].data == (*new_list)[p_new].data;
	}

	const void *get_change_payload(int p_old, int p_new) override {
		REQUIRE((*old_list)[p_old].id == (*new_list)[p_new].id);
		REQUIRE((*old_list)[p_old].data != (*new_list)[p_new].data);
		return (*new_list)[p_new].payload;
	}
};

struct ApplyingCallback : DiffListUpdateCallback {
	Vector<Item> &target;

	explicit ApplyingCallback(Vector<Item> &p_target) : target(p_target) {}

	void on_inserted(int p_position, int p_count) override {
		for (int i = 0; i < p_count; i++) {
			target.insert(p_position + i, Item(g_id_counter++, true, g_data_counter++));
		}
	}

	void on_removed(int p_position, int p_count) override {
		for (int i = 0; i < p_count; i++) {
			target.remove_at(p_position);
		}
	}

	void on_moved(int p_from_position, int p_to_position) override {
		Item item = target[p_from_position];
		target.remove_at(p_from_position);
		target.insert(p_to_position, item);
	}

	void on_changed(int p_position, int p_count, const void *p_payload) override {
		for (int i = 0; i < p_count; i++) {
			Item &existing = target.write[p_position + i];
			REQUIRE(existing.changed == false);
			REQUIRE(existing.new_item == false);
			REQUIRE(existing.payload == nullptr);
			existing.changed = true;
			existing.payload = p_payload;
		}
	}
};

// ---------------------------------------------------------------------------
// Test harness.

struct Harness {
	Vector<Item> before;
	Vector<Item> after;
	TestCallback callback;

	Harness() { callback.old_list = &before; callback.new_list = &after; }

	void init_with_size(int p_size) {
		g_id_counter = 0;
		g_data_counter = 0;
		g_payload_counter = 0;
		before.clear();
		after.clear();
		for (int i = 0; i < p_size; i++) {
			before.push_back(Item(g_id_counter++, false, g_data_counter++));
		}
		after = before;
	}

	void add(int p_index) { after.insert(p_index, Item(g_id_counter++, true, g_data_counter++)); }

	void delete_at(int p_index) { after.remove_at(p_index); }

	void move_op(int p_from, int p_to) {
		Item item = after[p_from];
		after.remove_at(p_from);
		after.insert(p_to, item);
	}

	void update(int p_index) {
		if (after[p_index].new_item) {
			return;
		}
		Item &item = after.write[p_index];
		item.changed = true;
		item.payload = nullptr;
		item.data = g_data_counter++;
	}

	void update_with_payload(int p_index) {
		if (after[p_index].new_item) {
			return;
		}
		Item &item = after.write[p_index];
		item.changed = true;
		item.payload = (const void *)(uintptr_t)(++g_payload_counter);
		item.data = g_data_counter++;
	}

	void duplicate_op(int p_pos, int p_to) {
		(void)p_to; // Android's duplicate() ignores the target; the item is re-inserted at p_pos.
		Item item = after[p_pos];
		after.insert(p_pos, item);
	}

	// ------------------------------------------------------------------
	// Verification (port of check() + assertEquals in DiffUtilTest.kt).

	bool contains_item(const Vector<Item> &p_list, const Item &p_item) const {
		for (const Item &it : p_list) {
			if (it == p_item) {
				return true;
			}
		}
		return false;
	}

	void remove_item(Vector<Item> &p_list, const Item &p_item) {
		for (int i = 0; i < p_list.size(); i++) {
			if (p_list[i] == p_item) {
				p_list.remove_at(i);
				return;
			}
		}
	}

	void verify_applied(const Vector<Item> &p_applied, const Vector<Item> &p_after) {
		REQUIRE(p_applied.size() == p_after.size());
		// id -> max # of copies allowed to show up as new items.
		Vector<int> duplicate_diffs;
		duplicate_diffs.resize((int)g_id_counter + 1);
		duplicate_diffs.fill(0);
		for (const Item &it : p_after) {
			if (!it.new_item) {
				int cur = duplicate_diffs[it.id];
				duplicate_diffs.write[it.id] = 1 + (cur == 0 ? 1 : cur);
			}
		}
		for (const Item &it : before) {
			duplicate_diffs.write[it.id] = duplicate_diffs[it.id] - 1;
		}
		for (int i = 0; i < p_after.size(); i++) {
			const Item &item = p_applied[i];
			const Item &expected = p_after[i];
			if (expected.new_item) {
				REQUIRE(item.new_item == true);
			} else if (duplicate_diffs[expected.id] > 0 && item.new_item) {
				duplicate_diffs.write[expected.id]--;
			} else if (expected.changed) {
				REQUIRE(item.new_item == false);
				REQUIRE(item.changed == true);
				REQUIRE(item.id == expected.id);
				REQUIRE(item.payload == expected.payload);
			} else {
				REQUIRE(item == expected);
			}
		}
	}

	void check() {
		DiffResultData result = DiffAlgorithm::calculate_diff(callback, true);

		Vector<Item> applied = before;
		ApplyingCallback applier(applied);
		result.dispatch_updates_to(applier);
		verify_applied(applied, after);

		// Position conversion: every mapped pair must share the same id, and removed
		// items must be absent from the other list.
		Vector<int> missing_before;
		Vector<Item> after_copy = after;
		for (int old_pos = 0; old_pos < before.size(); old_pos++) {
			int new_pos = result.convert_old_position_to_new(old_pos);
			if (new_pos != DiffResultData::NO_POSITION) {
				REQUIRE(before[old_pos].id == after[new_pos].id);
				remove_item(after_copy, after[new_pos]);
			} else {
				missing_before.push_back(old_pos);
			}
		}
		for (int i = 0; i < missing_before.size(); i++) {
			REQUIRE(!contains_item(after_copy, before[missing_before[i]]));
		}
		REQUIRE(result.convert_old_position_to_new(before.size()) == DiffResultData::NO_POSITION);
		REQUIRE(result.convert_old_position_to_new(-1) == DiffResultData::NO_POSITION);

		Vector<int> missing_after;
		Vector<Item> before_copy = before;
		for (int new_pos = 0; new_pos < after.size(); new_pos++) {
			int old_pos = result.convert_new_position_to_old(new_pos);
			if (old_pos != DiffResultData::NO_POSITION) {
				REQUIRE(after[new_pos].id == before[old_pos].id);
				remove_item(before_copy, before[old_pos]);
			} else {
				missing_after.push_back(new_pos);
			}
		}
		for (int i = 0; i < missing_after.size(); i++) {
			REQUIRE(!contains_item(before_copy, after[missing_after[i]]));
		}
		REQUIRE(result.convert_new_position_to_old(after.size()) == DiffResultData::NO_POSITION);
		REQUIRE(result.convert_new_position_to_old(-1) == DiffResultData::NO_POSITION);
	}
};

// Deterministic LCG so the fuzz test is reproducible.
int next_rand(unsigned int &p_state) {
	p_state = p_state * 1664525u + 1013904223u;
	return (int)(p_state >> 16) & 0x7fffffff;
}

} // namespace

// ---------------------------------------------------------------------------
// Test cases (port of the @Test methods in DiffUtilTest.kt).

TEST_CASE("test_no_change") {
	Harness h;
	h.init_with_size(5);
	h.check();
}

TEST_CASE("test_add_items") {
	Harness h;
	h.init_with_size(2);
	h.add(1);
	h.check();
}

TEST_CASE("test_gen2") {
	Harness h;
	h.init_with_size(5);
	h.add(5);
	h.delete_at(3);
	h.delete_at(1);
	h.check();
}

TEST_CASE("test_gen3") {
	Harness h;
	h.init_with_size(5);
	h.add(0);
	h.delete_at(1);
	h.delete_at(3);
	h.check();
}

TEST_CASE("test_gen4") {
	Harness h;
	h.init_with_size(5);
	h.add(5);
	h.add(1);
	h.add(4);
	h.add(4);
	h.check();
}

TEST_CASE("test_gen5") {
	Harness h;
	h.init_with_size(5);
	h.delete_at(0);
	h.delete_at(2);
	h.add(0);
	h.add(2);
	h.check();
}

TEST_CASE("test_gen6") {
	Harness h;
	h.init_with_size(2);
	h.delete_at(0);
	h.delete_at(0);
	h.check();
}

TEST_CASE("test_gen7") {
	Harness h;
	h.init_with_size(3);
	h.move_op(2, 0);
	h.delete_at(2);
	h.add(2);
	h.check();
}

TEST_CASE("test_gen8") {
	Harness h;
	h.init_with_size(3);
	h.delete_at(1);
	h.add(0);
	h.move_op(2, 0);
	h.check();
}

TEST_CASE("test_gen9") {
	Harness h;
	h.init_with_size(2);
	h.add(2);
	h.move_op(0, 2);
	h.check();
}

TEST_CASE("test_gen10") {
	Harness h;
	h.init_with_size(3);
	h.move_op(0, 1);
	h.move_op(1, 2);
	h.add(0);
	h.check();
}

TEST_CASE("test_gen11") {
	Harness h;
	h.init_with_size(4);
	h.move_op(2, 0);
	h.move_op(2, 3);
	h.check();
}

TEST_CASE("test_gen12") {
	Harness h;
	h.init_with_size(4);
	h.move_op(3, 0);
	h.move_op(2, 1);
	h.check();
}

TEST_CASE("test_gen13") {
	Harness h;
	h.init_with_size(4);
	h.move_op(3, 2);
	h.move_op(0, 3);
	h.check();
}

TEST_CASE("test_gen14") {
	Harness h;
	h.init_with_size(4);
	h.move_op(3, 2);
	h.add(4);
	h.move_op(0, 4);
	h.check();
}

TEST_CASE("test_gen15") {
	Harness h;
	h.init_with_size(1);
	h.update(0);
	h.update(0);
	h.update(0);
	h.check();
}

TEST_CASE("test_gen16") {
	Harness h;
	h.init_with_size(1);
	h.update(0);
	h.move_op(0, 0);
	h.move_op(0, 0);
	h.add(0);
	h.check();
}

TEST_CASE("test_gen17") {
	Harness h;
	h.init_with_size(2);
	h.move_op(1, 0);
	h.add(2);
	h.update(1);
	h.add(0);
	h.check();
}

TEST_CASE("test_gen18") {
	Harness h;
	h.init_with_size(2);
	h.update_with_payload(0);
	h.check();
}

TEST_CASE("test_gen19") {
	Harness h;
	h.init_with_size(3);
	h.move_op(1, 1);
	h.delete_at(2);
	h.move_op(0, 1);
	h.add(0);
	h.update(1);
	h.add(1);
	h.update_with_payload(2);
	h.add(1);
	h.delete_at(1);
	h.update_with_payload(3);
	h.add(2);
	h.move_op(2, 1);
	h.add(2);
	h.delete_at(2);
	h.delete_at(1);
	h.check();
}

TEST_CASE("test_one_item") {
	Harness h;
	h.init_with_size(1);
	h.check();
}

TEST_CASE("test_empty") {
	Harness h;
	h.init_with_size(0);
	h.check();
}

TEST_CASE("test_add1") {
	Harness h;
	h.init_with_size(1);
	h.add(1);
	h.check();
}

TEST_CASE("test_move1") {
	Harness h;
	h.init_with_size(3);
	h.move_op(0, 2);
	h.check();
}

TEST_CASE("test_update1") {
	Harness h;
	h.init_with_size(3);
	h.update(2);
	h.check();
}

TEST_CASE("test_update2") {
	Harness h;
	h.init_with_size(2);
	h.add(1);
	h.update(1);
	h.update(2);
	h.check();
}

TEST_CASE("test_disable_move_detection") {
	Harness h;
	h.init_with_size(5);
	h.move_op(0, 4);
	Vector<Item> applied = h.before;
	ApplyingCallback applier(applied);
	DiffResultData result = DiffAlgorithm::calculate_diff(h.callback, false);
	result.dispatch_updates_to(applier);
	REQUIRE(applied.size() == 5);
	REQUIRE(applied[4].new_item == true);
	REQUIRE(!h.contains_item(applied, h.before[0]));
}

TEST_CASE("duplicate") {
	Harness h;
	g_id_counter = 0;
	g_data_counter = 0;
	g_payload_counter = 0;
	h.before.push_back(Item(0, false, 100));
	h.before.push_back(Item(1, false, 200));
	h.after.push_back(h.before[0]);
	h.after.push_back(h.before[1]);
	h.after.push_back(Item(2, true, 300));
	h.after.push_back(h.before[1]);
	g_id_counter = 3; // New items created while applying start from id 3.
	h.check();
}

TEST_CASE("random_fuzz") {
	// Port of the (commented-out) testRandom with a fixed seed for reproducibility.
	unsigned int rand_state = 123456789u;
	for (int round = 0; round < 100; round++) {
		Harness h;
		h.init_with_size(next_rand(rand_state) % 8);
		int operation_count = 2 + next_rand(rand_state) % 40;
		for (int i = 0; i < operation_count; i++) {
			int op = next_rand(rand_state) % 6;
			switch (op) {
				case 0:
					h.add(next_rand(rand_state) % (h.after.size() + 1));
					break;
				case 1:
					if (!h.after.is_empty()) {
						h.delete_at(next_rand(rand_state) % h.after.size());
					}
					break;
				case 2:
					if (!h.after.is_empty()) {
						int from = next_rand(rand_state) % h.after.size();
						int to = next_rand(rand_state) % h.after.size();
						h.move_op(from, to);
					}
					break;
				case 3:
					if (!h.after.is_empty()) {
						h.update(next_rand(rand_state) % h.after.size());
					}
					break;
				case 4:
					if (!h.after.is_empty()) {
						h.update_with_payload(next_rand(rand_state) % h.after.size());
					}
					break;
				case 5:
					if (!h.after.is_empty()) {
						int pos = next_rand(rand_state) % h.after.size();
						h.duplicate_op(pos, next_rand(rand_state) % h.after.size());
					}
					break;
			}
		}
		h.check();
	}
}
