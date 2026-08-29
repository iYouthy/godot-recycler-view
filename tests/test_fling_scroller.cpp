// Pure tests for the SplineOverScroller fling port.

#include "doctest.h"

#include "fling_scroller.h"

#include <cmath>

using namespace godot;

// Runs a fling to completion in 16ms steps and returns the number of frames.
static int run_fling(FlingScroller &p_fling) {
	int frames = 0;
	while (p_fling.update(16.0)) {
		frames++;
		if (frames > 10000) {
			break;
		}
	}
	return frames;
}

TEST_CASE("fling distance and duration grow with velocity") {
	FlingScroller slow;
	slow.fling(0, 1000.0f, 0, 1000000);
	FlingScroller fast;
	fast.fling(0, 4000.0f, 0, 1000000);
	// Faster flings travel farther and last longer.
	CHECK(fast.get_final_position() > slow.get_final_position());
	CHECK(fast.get_duration() > slow.get_duration());
	// Both durations are in the tens/hundreds of ms range and finite.
	CHECK(slow.get_duration() > 0);
	CHECK(fast.get_duration() > 0);
}

TEST_CASE("fling direction is symmetric") {
	FlingScroller up;
	up.fling(0, 1000.0f, -1000000, 1000000);
	FlingScroller down;
	down.fling(0, -1000.0f, -1000000, 1000000);
	CHECK(up.get_final_position() == -down.get_final_position());
	CHECK(up.get_duration() == down.get_duration());
}

TEST_CASE("update progresses monotonically and settles on the target") {
	FlingScroller fling;
	fling.fling(0, 3000.0f, 0, 1000000);
	CHECK(!fling.is_finished());
	const int final_pos = fling.get_final_position();
	CHECK(final_pos > 0);

	int prev = fling.get_current_position();
	float prev_velocity = fling.get_current_velocity();
	bool still_going = true;
	while (still_going) {
		still_going = fling.update(16.0);
		const int cur = fling.get_current_position();
		CHECK(cur >= prev); // never moves backwards
		prev = cur;
		const float vel = fling.get_current_velocity();
		CHECK(vel <= prev_velocity + 0.001f); // decelerates monotonically
		prev_velocity = vel;
	}
	CHECK(fling.is_finished());
	CHECK(fling.get_current_position() == final_pos);
}

TEST_CASE("fling clamps to the boundary and settles there") {
	FlingScroller fling;
	fling.fling(0, 3000.0f, 0, 100); // natural distance >> 100
	CHECK(fling.get_final_position() == 100);
	run_fling(fling);
	CHECK(fling.is_finished());
	CHECK(fling.get_current_position() == 100);
}

TEST_CASE("zero velocity fling settles immediately") {
	FlingScroller fling;
	fling.fling(50, 0.0f, 0, 1000);
	CHECK(fling.get_final_position() == 50);
	CHECK(!fling.is_finished());
	// First nonzero update settles it without moving.
	CHECK(!fling.update(16.0));
	CHECK(fling.is_finished());
	CHECK(fling.get_current_position() == 50);
}

TEST_CASE("negative velocity flings backward") {
	FlingScroller fling;
	fling.fling(500, -2000.0f, 0, 1000000);
	CHECK(fling.get_final_position() < 500);
	run_fling(fling);
	CHECK(fling.get_current_position() == fling.get_final_position());
}

TEST_CASE("stop aborts the fling at the current position") {
	FlingScroller fling;
	fling.fling(0, 3000.0f, 0, 1000000);
	for (int i = 0; i < 5; i++) {
		fling.update(16.0);
	}
	const int parked = fling.get_current_position();
	fling.stop();
	CHECK(fling.is_finished());
	CHECK(fling.get_current_position() == parked);
	CHECK(fling.update(16.0)); // update after stop is a no-op, still finished
	CHECK(fling.is_finished());
}

TEST_CASE("spline table is anchored, monotonic and strictly inside (0,1)") {
	const float *table = FlingScroller::spline_position();
	// The port reproduces the original's float bisection, so entry 0 is a tiny
	// nonzero (about 2e-5) rather than exactly 0.
	CHECK(table[0] >= 0.0f);
	CHECK(table[0] < 0.001f);
	CHECK(table[100] == 1.0f);
	for (int i = 0; i < 100; i++) {
		CHECK(table[i] < table[i + 1]);
	}
	// The curve rises quickly then flattens (deceleration), so the mid point
	// is well above the linear t value.
	CHECK(table[50] > 0.5f);
}

TEST_CASE("fling lands close to the spline distance") {
	// The spline is anchored at (0,0) and (1,1), so the final position must
	// equal start + spline_distance = final, which update lands on exactly.
	FlingScroller fling;
	fling.fling(0, 1500.0f, 0, 1000000);
	const int final_pos = fling.get_final_position();
	run_fling(fling);
	CHECK(fling.get_current_position() == final_pos);
	// Sanity on the magnitude: 1500 px/s should travel a few hundred px.
	CHECK(final_pos > 100);
	CHECK(final_pos < 10000);
}

TEST_CASE("was_clamped reflects boundary spills") {
	// Natural target inside the bounds: not clamped.
	FlingScroller inside;
	inside.fling(100, 1500.0f, 0, 1000000);
	CHECK(!inside.was_clamped());
	// Natural target crosses the max: clamped, and the final lands on max.
	FlingScroller over;
	over.fling(100, 5000.0f, 0, 300);
	CHECK(over.was_clamped());
	CHECK(over.get_final_position() == 300);
	// Natural target crosses the min (negative direction): clamped.
	FlingScroller under;
	under.fling(50, -5000.0f, 0, 1000000);
	CHECK(under.was_clamped());
	CHECK(under.get_final_position() == 0);
	// A new fling resets the flag.
	inside.fling(100, 5000.0f, 0, 300);
	CHECK(inside.was_clamped());
	inside.fling(100, 1500.0f, 0, 1000000);
	CHECK(!inside.was_clamped());
}

TEST_CASE("predict_end_distance matches the unclamped spline distance") {
	// A fling inside the bounds settles exactly at start + predict_end_distance.
	FlingScroller fling;
	fling.fling(0, 3000.0f, 0, 1000000);
	CHECK(fling.get_final_position() == FlingScroller::predict_end_distance(3000.0f));
	// Zero velocity travels nothing.
	CHECK(FlingScroller::predict_end_distance(0.0f) == 0);
	// Direction is symmetric.
	CHECK(FlingScroller::predict_end_distance(2000.0f) == -FlingScroller::predict_end_distance(-2000.0f));
	// Positive velocity scrolls forward (later positions).
	CHECK(FlingScroller::predict_end_distance(2000.0f) > 0);
}
