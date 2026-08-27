// Pure tests for the drag-velocity estimator.

#include "doctest.h"

#include "velocity_tracker.h"

using namespace godot;

TEST_CASE("constant velocity is recovered from samples") {
	VelocityTracker tracker;
	tracker.add_sample(0.0f, 0.0);
	tracker.add_sample(16.0f, 16.0);
	tracker.add_sample(32.0f, 32.0);
	tracker.add_sample(48.0f, 48.0);
	tracker.add_sample(64.0f, 64.0);
	// Newest sample vs oldest inside the 50ms window: 64px over 48ms.
	CHECK(tracker.get_velocity() == doctest::Approx(1000.0f));
}

TEST_CASE("old samples outside the window are ignored") {
	VelocityTracker tracker;
	tracker.add_sample(0.0f, 0.0); // stale, far outside the window
	tracker.add_sample(100.0f, 90.0);
	tracker.add_sample(190.0f, 100.0);
	// Only the two in-window samples count: 90px over 10ms = 9000 px/s.
	CHECK(tracker.get_velocity() == doctest::Approx(9000.0f));
}

TEST_CASE("acceleration is reflected in the estimate") {
	VelocityTracker tracker;
	tracker.add_sample(0.0f, 0.0);
	tracker.add_sample(20.0f, 10.0);
	tracker.add_sample(60.0f, 20.0); // sped up: last 40px over 10ms
	tracker.add_sample(120.0f, 30.0);
	// Newest vs oldest in window: 120px over 30ms.
	CHECK(tracker.get_velocity() == doctest::Approx(4000.0f));
}

TEST_CASE("negative direction") {
	VelocityTracker tracker;
	tracker.add_sample(100.0f, 0.0);
	tracker.add_sample(80.0f, 10.0);
	tracker.add_sample(40.0f, 20.0);
	CHECK(tracker.get_velocity() == doctest::Approx(-3000.0f));
}

TEST_CASE("samples sharing a timestamp yield zero velocity") {
	VelocityTracker tracker;
	tracker.add_sample(0.0f, 16.0);
	tracker.add_sample(50.0f, 16.0); // same frame, no time spread
	CHECK(tracker.get_velocity() == 0.0f);
}

TEST_CASE("fewer than two samples yields zero") {
	VelocityTracker tracker;
	CHECK(tracker.get_velocity() == 0.0f);
	tracker.add_sample(10.0f, 0.0);
	CHECK(tracker.get_velocity() == 0.0f);
}

TEST_CASE("clear resets the estimator") {
	VelocityTracker tracker;
	tracker.add_sample(0.0f, 0.0);
	tracker.add_sample(100.0f, 16.0);
	CHECK(tracker.get_velocity() != 0.0f);
	tracker.clear();
	CHECK(tracker.get_velocity() == 0.0f);
}
