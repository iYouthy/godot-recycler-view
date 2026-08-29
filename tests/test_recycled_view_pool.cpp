// Tests for the RecycledViewPool port (no Android JVM reference test exists).

#include "doctest.h"

#include "recycled_view_pool.h"

using namespace godot;

TEST_CASE("pool_put_get_lifo") {
	RecycledViewPool pool;
	int a = 1;
	int b = 2;
	pool.put_recycled_view(&a, 0);
	pool.put_recycled_view(&b, 0);
	REQUIRE(pool.get_recycled_view(0) == &b); // Most recent first.
	REQUIRE(pool.get_recycled_view(0) == &a);
	REQUIRE(pool.get_recycled_view(0) == nullptr);
}

TEST_CASE("pool_capacity_bounds_by_default") {
	RecycledViewPool pool;
	// Default capacity is 5 per type (Android's DEFAULT_MAX_SCRAP); putting more
	// trims the oldest.
	for (int i = 0; i < 25; i++) {
		pool.put_recycled_view((void *)(intptr_t)(i + 1), 0);
	}
	REQUIRE(pool.get_recycled_view_count(0) == 5);
	REQUIRE(pool.size() == 5);
}

TEST_CASE("pool_set_max_recycled_views_trims") {
	RecycledViewPool pool;
	for (int i = 0; i < 5; i++) {
		pool.put_recycled_view((void *)(intptr_t)(i + 1), 0);
	}
	pool.set_max_recycled_views(0, 2);
	REQUIRE(pool.get_recycled_view_count(0) == 2);
	// Lowering below current capacity discards from the end (oldest first).
	pool.set_max_recycled_views(0, 0);
	REQUIRE(pool.get_recycled_view_count(0) == 0);
	REQUIRE(pool.get_recycled_view(0) == nullptr);
}

TEST_CASE("pool_types_are_isolated") {
	RecycledViewPool pool;
	int a = 1;
	int b = 2;
	pool.put_recycled_view(&a, 0);
	pool.put_recycled_view(&b, 1);
	REQUIRE(pool.get_recycled_view_count(0) == 1);
	REQUIRE(pool.get_recycled_view_count(1) == 1);
	REQUIRE(pool.get_recycled_view(0) == &a);
	REQUIRE(pool.get_recycled_view(1) == &b);
	REQUIRE(pool.get_recycled_view_count(0) == 0);
	REQUIRE(pool.get_recycled_view_count(1) == 0);
	REQUIRE(pool.get_recycled_view(9) == nullptr);
	REQUIRE(pool.get_recycled_view_count(9) == 0);
}

TEST_CASE("pool_clear_preserves_capacity") {
	RecycledViewPool pool;
	for (int i = 0; i < 3; i++) {
		pool.put_recycled_view((void *)(intptr_t)(i + 1), 0);
	}
	pool.set_max_recycled_views(0, 7);
	pool.clear();
	REQUIRE(pool.size() == 0);
	// Capacity setting survives clear.
	pool.put_recycled_view((void *)(intptr_t)1, 0);
	pool.put_recycled_view((void *)(intptr_t)2, 0);
	pool.put_recycled_view((void *)(intptr_t)3, 0);
	pool.put_recycled_view((void *)(intptr_t)4, 0);
	pool.put_recycled_view((void *)(intptr_t)5, 0);
	pool.put_recycled_view((void *)(intptr_t)6, 0);
	pool.put_recycled_view((void *)(intptr_t)7, 0);
	pool.put_recycled_view((void *)(intptr_t)8, 0); // Should be discarded (max 7).
	REQUIRE(pool.get_recycled_view_count(0) == 7);
}

TEST_CASE("pool_running_average") {
	RecycledViewPool pool;
	pool.factor_in_create_time(0, 100);
	REQUIRE(pool.will_create_in_time(0, 0, 101));
	REQUIRE_FALSE(pool.will_create_in_time(0, 100, 101));
	// Second sample blends toward the new value.
	pool.factor_in_create_time(0, 200);
	REQUIRE(pool.get_create_running_average_ns(0) > 100);
	// Unknown type has zero average and always passes.
	REQUIRE(pool.will_create_in_time(9, 0, 1));
	REQUIRE(pool.will_bind_in_time(9, 0, 1));
}

TEST_CASE("pool_attach_detach_count") {
	RecycledViewPool pool;
	REQUIRE(pool.attach_count() == 0);
	pool.attach();
	pool.attach();
	REQUIRE(pool.attach_count() == 2);
	pool.detach();
	REQUIRE(pool.attach_count() == 1);
}
