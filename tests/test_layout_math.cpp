// Pure tests for the layout math shared by the layout managers.

#include "doctest.h"

#include "layout_math.h"

using namespace godot;

TEST_CASE("upper_bound / lower_bound on a cumulative offset table") {
	Vector<int> offsets;
	// Cumulative heights: [0, 40, 80, 120, 160] (5 fixed-height items).
	offsets.push_back(0);
	offsets.push_back(40);
	offsets.push_back(80);
	offsets.push_back(120);
	offsets.push_back(160);

	CHECK(upper_bound(offsets, 5, 0) == 1);   // first offset > 0
	CHECK(upper_bound(offsets, 5, 39) == 1);
	CHECK(upper_bound(offsets, 5, 40) == 2);  // first offset > 40
	CHECK(upper_bound(offsets, 5, 200) == 5); // none -> size
	CHECK(lower_bound(offsets, 5, 0) == 0);   // first offset >= 0
	CHECK(lower_bound(offsets, 5, 40) == 1);
	CHECK(lower_bound(offsets, 5, 161) == 5); // none -> size
}

TEST_CASE("upper_bound / lower_bound tolerate zero heights") {
	Vector<int> offsets;
	offsets.push_back(0);
	offsets.push_back(0); // zero-height item
	offsets.push_back(40);
	offsets.push_back(40); // another zero-height item
	offsets.push_back(80);

	CHECK(upper_bound(offsets, 5, 0) == 2);   // first offset > 0 (skips zeros)
	CHECK(lower_bound(offsets, 5, 40) == 2);
	CHECK(upper_bound(offsets, 5, 40) == 4);
}

TEST_CASE("calculate_item_borders divides evenly") {
	Vector<int> borders = calculate_item_borders(3, 99);
	CHECK(borders.size() == 4);
	CHECK(borders[0] == 0);
	CHECK(borders[3] == 99); // last border is the total
	// 99 / 3 = 33 exactly, no remainder.
	CHECK(borders[1] == 33);
	CHECK(borders[2] == 66);
}

TEST_CASE("calculate_item_borders distributes remainder pixels") {
	// 640 / 3 = 213 remainder 1: the last cell absorbs the extra pixel.
	Vector<int> b = calculate_item_borders(3, 640);
	CHECK(b[0] == 0);
	CHECK(b[1] == 213);
	CHECK(b[2] == 426);
	CHECK(b[3] == 640);

	// 102 / 5 = 20 remainder 2: cells 2 and 5 get an extra pixel.
	Vector<int> c = calculate_item_borders(5, 102);
	CHECK(c[0] == 0);
	CHECK(c[1] == 20);
	CHECK(c[2] == 41);
	CHECK(c[3] == 61);
	CHECK(c[4] == 81);
	CHECK(c[5] == 102);
}

TEST_CASE("calculate_item_borders single span") {
	Vector<int> borders = calculate_item_borders(1, 360);
	CHECK(borders.size() == 2);
	CHECK(borders[0] == 0);
	CHECK(borders[1] == 360);
}
