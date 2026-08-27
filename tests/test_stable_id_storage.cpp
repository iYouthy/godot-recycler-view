// Tests for the StableIdStorage port (no Android JVM reference test exists).

#include "doctest.h"

#include "stable_id_storage.h"

using namespace godot;

TEST_CASE("no_stable_id_storage_returns_no_id") {
	NoStableIdStorage storage;
	StableIdLookup *lookup = storage.create_stable_id_lookup();
	REQUIRE(lookup->local_to_global(0) == NO_STABLE_ID);
	REQUIRE(lookup->local_to_global(42) == NO_STABLE_ID);
	REQUIRE(lookup->local_to_global(-7) == NO_STABLE_ID);
	// Lookup is stable across calls.
	REQUIRE(storage.create_stable_id_lookup() == storage.create_stable_id_lookup());
}

TEST_CASE("shared_pool_stable_id_storage_passes_through") {
	SharedPoolStableIdStorage storage;
	StableIdLookup *lookup = storage.create_stable_id_lookup();
	REQUIRE(lookup->local_to_global(0) == 0);
	REQUIRE(lookup->local_to_global(42) == 42);
	REQUIRE(lookup->local_to_global(-7) == -7);
}

TEST_CASE("isolated_stable_id_storage_assigns_globals") {
	IsolatedStableIdStorage storage;
	StableIdLookup *lookup = storage.create_stable_id_lookup();
	// Same local id maps consistently.
	int64_t g0 = lookup->local_to_global(0);
	REQUIRE(lookup->local_to_global(0) == g0);
	// Different local ids get distinct global ids.
	int64_t g1 = lookup->local_to_global(1);
	REQUIRE(g1 != g0);
	delete lookup;
}

TEST_CASE("isolated_stable_id_storage_lookups_share_id_domain") {
	IsolatedStableIdStorage storage;
	StableIdLookup *l1 = storage.create_stable_id_lookup();
	StableIdLookup *l2 = storage.create_stable_id_lookup();
	int64_t a = l1->local_to_global(0);
	int64_t b = l2->local_to_global(0); // Same local id in a different adapter.
	int64_t c = l1->local_to_global(5);
	REQUIRE(a != b);
	REQUIRE(a != c);
	REQUIRE(b != c);
	delete l1;
	delete l2;
}

TEST_CASE("isolated_stable_id_storage_lookup_is_idempotent") {
	IsolatedStableIdStorage storage;
	StableIdLookup *lookup = storage.create_stable_id_lookup();
	for (int i = 0; i < 100; i++) {
		REQUIRE(lookup->local_to_global(i) == lookup->local_to_global(i));
	}
	// All 100 local ids map to unique globals in [0, 100).
	for (int i = 0; i < 100; i++) {
		int64_t g = lookup->local_to_global(i);
		REQUIRE(g >= 0);
		REQUIRE(g < 100);
	}
	delete lookup;
}
