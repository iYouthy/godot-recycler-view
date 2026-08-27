// Port of androidx OpReorderTest.java exercising the OpReorderer algorithm
// through the C++ interfaces.

#include "doctest.h"

#include "op_reorderer.h"

using namespace godot;

namespace {

struct TestItem {
	int id;
	int version;

	static int id_counter;

	TestItem() :
			id(0), version(0) {}

	TestItem(int p_id, int p_version) :
			id(p_id), version(p_version) {}

	static TestItem create() { return TestItem(id_counter++, 1); }
	static TestItem clone(const TestItem &p_other) { return TestItem(p_other.id, p_other.version); }

	static void assert_identical(const TestItem &p_a, const TestItem &p_b) {
		REQUIRE(p_a.id == p_b.id);
		REQUIRE(p_a.version == p_b.version);
	}
};

int TestItem::id_counter = 0;

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
	// Callback must be declared before the reorderer so it is constructed first.
	struct Callback : OpReordererCallback {
		Vector<UpdateOp *> *recycled;

		explicit Callback(Vector<UpdateOp *> *p_recycled) :
				recycled(p_recycled) {}

		UpdateOp *obtain_update_op(int p_cmd, int p_start_position, int p_item_count, const void *p_payload) override {
			return new UpdateOp(p_cmd, p_start_position, p_item_count, p_payload);
		}

		void recycle_update_op(UpdateOp *p_op) override {
			recycled->push_back(p_op);
		}
	};

	Callback callback;
	OpReorderer reorderer;

	Vector<UpdateOp *> update_ops;
	Vector<TestItem> added_items;
	Vector<TestItem> removed_items;
	Vector<UpdateOp *> recycled_ops;

	int item_count = 0;
	int updated_item_count = 0;

	Harness() :
			callback(&recycled_ops), reorderer(&callback) {}

	void clean_state() {
		update_ops.clear();
		added_items.clear();
		removed_items.clear();
		recycled_ops.clear();
		TestItem::id_counter = 0;
	}

	void setup(int p_count) {
		item_count = p_count;
		updated_item_count = p_count;
	}

	UpdateOp *record(UpdateOp *p_op) {
		update_ops.push_back(p_op);
		return p_op;
	}

	UpdateOp *rm(int p_start, int p_count) {
		updated_item_count -= p_count;
		return record(new UpdateOp(UpdateOp::REMOVE, p_start, p_count, nullptr));
	}

	UpdateOp *mv(int p_from, int p_to) {
		return record(new UpdateOp(UpdateOp::MOVE, p_from, p_to, nullptr));
	}

	UpdateOp *add(int p_start, int p_count) {
		updated_item_count += p_count;
		return record(new UpdateOp(UpdateOp::ADD, p_start, p_count, nullptr));
	}

	UpdateOp *up(int p_start, int p_count) {
		return record(new UpdateOp(UpdateOp::UPDATE, p_start, p_count, nullptr));
	}

	void rand_op(int p_cmd, unsigned int &p_state) {
		switch (p_cmd) {
			case UpdateOp::REMOVE:
				if (updated_item_count > 1) {
					int s = next_int(p_state, updated_item_count - 1);
					int len = next_int(p_state, updated_item_count - s);
					if (len < 1) {
						len = 1;
					}
					rm(s, len);
				}
				break;
			case UpdateOp::ADD: {
				int s = updated_item_count == 0 ? 0 : next_int(p_state, updated_item_count);
				add(s, next_int(p_state, 50));
				break;
			}
			case UpdateOp::MOVE:
				if (updated_item_count >= 2) {
					int from = next_int(p_state, updated_item_count);
					int to;
					do {
						to = next_int(p_state, updated_item_count);
					} while (to == from);
					mv(from, to);
				}
				break;
			case UpdateOp::UPDATE:
				if (updated_item_count > 1) {
					int s = next_int(p_state, updated_item_count - 1);
					int len = next_int(p_state, updated_item_count - s);
					if (len < 1) {
						len = 1;
					}
					up(s, len);
				}
				break;
		}
	}

	void ordered_random(int p_cmd1, int p_cmd2, unsigned int &p_state) {
		rand_op(p_cmd1, p_state);
		rand_op(p_cmd2, p_state);
	}

	Vector<UpdateOp *> rewrite_ops(const Vector<UpdateOp *> &p_ops) {
		Vector<UpdateOp *> copy;
		for (int i = 0; i < p_ops.size(); i++) {
			copy.push_back(new UpdateOp(p_ops[i]->cmd, p_ops[i]->position_start, p_ops[i]->item_count, nullptr));
		}
		reorderer.reorder_ops(copy);
		return copy;
	}

	void apply(Vector<TestItem> &p_items, const Vector<UpdateOp *> &p_ops, Vector<TestItem> &r_added, Vector<TestItem> &r_removed) {
		for (int oi = 0; oi < p_ops.size(); oi++) {
			UpdateOp *op = p_ops[oi];
			switch (op->cmd) {
				case UpdateOp::ADD:
					for (int i = 0; i < op->item_count; i++) {
						TestItem new_item = TestItem::create();
						r_added.push_back(new_item);
						p_items.insert(op->position_start + i, new_item);
					}
					break;
				case UpdateOp::REMOVE:
					for (int i = 0; i < op->item_count; i++) {
						r_removed.push_back(p_items[op->position_start]);
						p_items.remove_at(op->position_start);
					}
					break;
				case UpdateOp::MOVE: {
					int from = op->position_start;
					int to = op->item_count;
					TestItem item = p_items[from];
					p_items.remove_at(from);
					p_items.insert(to, item);
					break;
				}
				case UpdateOp::UPDATE:
					for (int i = 0; i < op->item_count; i++) {
						p_items.write[op->position_start + i].version++;
					}
					break;
			}
		}
	}

	void assert_all_moves_at_the_end(const Vector<UpdateOp *> &p_ops) {
		bool found_move = false;
		for (int i = 0; i < p_ops.size(); i++) {
			if (p_ops[i]->cmd == UpdateOp::MOVE) {
				found_move = true;
			} else {
				REQUIRE_FALSE(found_move);
			}
		}
	}

	void assert_lists_identical(const Vector<TestItem> &p_items, const Vector<TestItem> &p_clones) {
		REQUIRE(p_items.size() == p_clones.size());
		for (int i = 0; i < p_items.size(); i++) {
			TestItem::assert_identical(p_items[i], p_clones[i]);
		}
	}

	void assert_has_the_same_items(Vector<TestItem> p_items, Vector<TestItem> &p_clones) {
		REQUIRE(p_items.size() == p_clones.size());
		for (int i = 0; i < p_items.size(); i++) {
			for (int j = 0; j < p_clones.size(); j++) {
				if (p_items[i].id == p_clones[j].id && p_items[i].version == p_clones[j].version) {
					p_clones.remove_at(j);
					break;
				}
			}
		}
		REQUIRE(p_clones.is_empty());
	}

	void assert_recycled_ops_are_not_reused(const Vector<UpdateOp *> &p_ops) {
		for (int i = 0; i < p_ops.size(); i++) {
			REQUIRE_FALSE(recycled_ops.has(p_ops[i]));
		}
	}

	void process() {
		Vector<TestItem> items;
		for (int i = 0; i < item_count; i++) {
			items.push_back(TestItem::create());
		}
		Vector<TestItem> clones;
		for (int i = 0; i < item_count; i++) {
			clones.push_back(TestItem::clone(items[i]));
		}
		Vector<UpdateOp *> rewritten = rewrite_ops(update_ops);

		assert_all_moves_at_the_end(rewritten);

		apply(items, update_ops, added_items, removed_items);
		Vector<TestItem> original_added = added_items;
		Vector<TestItem> original_removed = removed_items;
		if (!original_added.is_empty()) {
			TestItem::id_counter = original_added[0].id;
		}
		added_items.clear();
		removed_items.clear();
		apply(clones, rewritten, added_items, removed_items);

		assert_lists_identical(items, clones);
		assert_has_the_same_items(original_added, added_items);
		assert_has_the_same_items(original_removed, removed_items);

		assert_recycled_ops_are_not_reused(rewritten);
	}
};

} // namespace

// ---------------------------------------------------------------------------
// Test cases (port of the @Test methods in OpReorderTest.java).

TEST_CASE("test_move_removed") {
	Harness h;
	h.clean_state();
	h.setup(10);
	h.mv(3, 8);
	h.rm(7, 3);
	h.process();
}

TEST_CASE("test_move_remove") {
	Harness h;
	h.clean_state();
	h.setup(10);
	h.mv(3, 8);
	h.rm(3, 5);
	h.process();
}

TEST_CASE("test_1") {
	Harness h;
	h.clean_state();
	h.setup(10);
	h.mv(3, 5);
	h.rm(3, 4);
	h.process();
}

TEST_CASE("test_2") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(1, 3);
	h.rm(1, 1);
	h.process();
}

TEST_CASE("test_3") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(0, 4);
	h.rm(2, 1);
	h.process();
}

TEST_CASE("test_4") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(3, 0);
	h.rm(3, 1);
	h.process();
}

TEST_CASE("test_5") {
	Harness h;
	h.clean_state();
	h.setup(10);
	h.mv(8, 1);
	h.rm(6, 3);
	h.process();
}

TEST_CASE("test_6") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(1, 3);
	h.rm(0, 3);
	h.process();
}

TEST_CASE("test_7") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(3, 4);
	h.rm(3, 1);
	h.process();
}

TEST_CASE("test_8") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(4, 3);
	h.rm(3, 1);
	h.process();
}

TEST_CASE("test_9") {
	Harness h;
	h.clean_state();
	h.setup(5);
	h.mv(2, 0);
	h.rm(2, 2);
	h.process();
}

// ---------------------------------------------------------------------------
// Direct swap tests.

TEST_CASE("test_swap_move_remove_1") {
	Harness h;
	h.clean_state();
	h.mv(10, 15);
	h.rm(2, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.mv(7, 12) == *h.update_ops[1]);
	REQUIRE(*h.rm(2, 3) == *h.update_ops[0]);
}

TEST_CASE("test_swap_move_remove_2") {
	Harness h;
	h.clean_state();
	h.mv(3, 8);
	h.rm(4, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(5, 2) == *h.update_ops[0]);
	REQUIRE(*h.mv(3, 6) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_3") {
	Harness h;
	h.clean_state();
	h.mv(3, 8);
	h.rm(3, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(4, 2) == *h.update_ops[0]);
	REQUIRE(*h.mv(3, 6) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_4") {
	Harness h;
	h.clean_state();
	h.mv(3, 8);
	h.rm(2, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 3);
	REQUIRE(*h.rm(4, 2) == *h.update_ops[0]);
	REQUIRE(*h.rm(2, 1) == *h.update_ops[1]);
	REQUIRE(*h.mv(2, 5) == *h.update_ops[2]);
}

TEST_CASE("test_swap_move_remove_5") {
	Harness h;
	h.clean_state();
	h.mv(3, 0);
	h.rm(2, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 3);
	REQUIRE(*h.rm(4, 1) == *h.update_ops[0]);
	REQUIRE(*h.rm(1, 2) == *h.update_ops[1]);
	REQUIRE(*h.mv(1, 0) == *h.update_ops[2]);
}

TEST_CASE("test_swap_move_remove_6") {
	Harness h;
	h.clean_state();
	h.mv(3, 10);
	h.rm(2, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 3);
	REQUIRE(*h.rm(4, 2) == *h.update_ops[0]);
	REQUIRE(*h.rm(2, 1) == *h.update_ops[1]);
	// The third op is an internal extra remove, not asserted by Android.
	REQUIRE(h.update_ops[2]->cmd == UpdateOp::MOVE);
}

TEST_CASE("test_swap_move_remove_7") {
	Harness h;
	h.clean_state();
	h.mv(3, 2);
	h.rm(6, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(6, 2) == *h.update_ops[0]);
	REQUIRE(*h.mv(3, 2) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_8") {
	Harness h;
	h.clean_state();
	h.mv(3, 4);
	h.rm(3, 1);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 1);
	REQUIRE(*h.rm(4, 1) == *h.update_ops[0]);
}

TEST_CASE("test_swap_move_remove_9") {
	Harness h;
	h.clean_state();
	h.mv(3, 4);
	h.rm(4, 1);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 1);
	REQUIRE(*h.rm(3, 1) == *h.update_ops[0]);
}

TEST_CASE("test_swap_move_remove_10") {
	Harness h;
	h.clean_state();
	h.mv(1, 3);
	h.rm(0, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(2, 2) == *h.update_ops[0]);
	REQUIRE(*h.rm(0, 1) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_11") {
	Harness h;
	h.clean_state();
	h.mv(3, 8);
	h.rm(7, 3);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(3, 1) == *h.update_ops[0]);
	REQUIRE(*h.rm(7, 2) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_12") {
	Harness h;
	h.clean_state();
	h.mv(1, 3);
	h.rm(2, 1);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(3, 1) == *h.update_ops[0]);
	REQUIRE(*h.mv(1, 2) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_13") {
	Harness h;
	h.clean_state();
	h.mv(1, 3);
	h.rm(1, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 1);
	REQUIRE(*h.rm(2, 2) == *h.update_ops[0]);
}

TEST_CASE("test_swap_move_remove_14") {
	Harness h;
	h.clean_state();
	h.mv(4, 2);
	h.rm(3, 1);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(2, 1) == *h.update_ops[0]);
	REQUIRE(*h.mv(2, 3) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_remove_15") {
	Harness h;
	h.clean_state();
	h.mv(4, 2);
	h.rm(3, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 1);
	REQUIRE(*h.rm(2, 2) == *h.update_ops[0]);
}

TEST_CASE("test_swap_move_remove_16") {
	Harness h;
	h.clean_state();
	h.mv(2, 3);
	h.rm(1, 2);
	h.reorderer.swap_move_remove(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.rm(3, 1) == *h.update_ops[0]);
	REQUIRE(*h.rm(1, 1) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_update_0") {
	Harness h;
	h.clean_state();
	h.mv(1, 3);
	h.up(1, 2);
	h.reorderer.swap_move_update(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 2);
	REQUIRE(*h.up(2, 2) == *h.update_ops[0]);
	REQUIRE(*h.mv(1, 3) == *h.update_ops[1]);
}

TEST_CASE("test_swap_move_update_1") {
	Harness h;
	h.clean_state();
	h.mv(0, 2);
	h.up(0, 4);
	h.reorderer.swap_move_update(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 3);
	REQUIRE(*h.up(0, 1) == *h.update_ops[0]);
	REQUIRE(*h.up(1, 3) == *h.update_ops[1]);
	REQUIRE(*h.mv(0, 2) == *h.update_ops[2]);
}

TEST_CASE("test_swap_move_update_2") {
	Harness h;
	h.clean_state();
	h.mv(2, 0);
	h.up(1, 3);
	h.reorderer.swap_move_update(h.update_ops, 0, h.update_ops[0], 1, h.update_ops[1]);
	REQUIRE(h.update_ops.size() == 3);
	REQUIRE(*h.up(3, 1) == *h.update_ops[0]);
	REQUIRE(*h.up(0, 2) == *h.update_ops[1]);
	REQUIRE(*h.mv(2, 0) == *h.update_ops[2]);
}

// ---------------------------------------------------------------------------
// Random fuzz tests (deterministic seeds).

TEST_CASE("test_random") {
	Harness h;
	h.clean_state();
	h.setup(50);
	unsigned int state = 123456789u;
	for (int i = 0; i < 150; i++) {
		h.clean_state();
		h.setup(50);
		for (int j = 0; j < 50; j++) {
			h.rand_op(next_int(state, next_int(state, 4)), state);
		}
		h.process();
	}
}

TEST_CASE("test_random_move_remove") {
	Harness h;
	unsigned int state = 987654321u;
	for (int i = 0; i < 1000; i++) {
		h.clean_state();
		h.setup(5);
		h.ordered_random(UpdateOp::MOVE, UpdateOp::REMOVE, state);
		h.process();
	}
}

TEST_CASE("test_random_move_add") {
	Harness h;
	unsigned int state = 135791113u;
	for (int i = 0; i < 1000; i++) {
		h.clean_state();
		h.setup(5);
		h.ordered_random(UpdateOp::MOVE, UpdateOp::ADD, state);
		h.process();
	}
}

TEST_CASE("test_random_move_update") {
	Harness h;
	unsigned int state = 2468101214u;
	for (int i = 0; i < 1000; i++) {
		h.clean_state();
		h.setup(5);
		h.ordered_random(UpdateOp::MOVE, UpdateOp::UPDATE, state);
		h.process();
	}
}
