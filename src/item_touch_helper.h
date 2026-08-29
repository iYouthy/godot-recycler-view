#pragma once

#include "item_touch_helper_callback.h"
#include "view_holder.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {

class RecyclerView;

// Port of ItemTouchHelper: long-press to drag-reorder, horizontal swipe to
// dismiss. Attach to a RecyclerView with attach_to_recycler_view() after
// setting a callback. During an active gesture it intercepts the RV's
// gui_input events before scroll arbitration and drives the selected holder's
// control position directly ("pinned under the finger": render stays at the
// slot position captured at grab time plus the finger delta, exactly like
// Android's translation model). A self-drawn RecoverAnimation settles the item
// on drop (drag) or slides it out / bounces it back (swipe).
class ItemTouchHelper : public RefCounted {
	GDCLASS(ItemTouchHelper, RefCounted)

protected:
	static void _bind_methods();

public:
	enum ActionState {
		ACTION_STATE_IDLE = 0,
		ACTION_STATE_SWIPE = 1,
		ACTION_STATE_DRAG = 2,
	};

	enum AnimationType {
		ANIMATION_TYPE_SWIPE_SUCCESS = 1 << 1,
		ANIMATION_TYPE_SWIPE_CANCEL = 1 << 2,
		ANIMATION_TYPE_DRAG = 1 << 3,
	};

	ItemTouchHelper();
	~ItemTouchHelper() override;

	// Static: packs drag and swipe direction flags into the movement-flags int
	// the callback returns (Android ItemTouchHelper.makeMovementFlags).
	static int make_movement_flags(int p_drag_flags, int p_swipe_flags);

	void set_callback(const Ref<ItemTouchHelperCallback> &p_callback);
	Ref<ItemTouchHelperCallback> get_callback() const { return m_callback; }
	void attach_to_recycler_view(RecyclerView *p_recycler_view);
	void detach();
	// Called by the RecyclerView destructor so a dangling rv pointer is never
	// dereferenced after the RV is freed.
	void on_recycler_view_destroyed();

	// Test/observer helpers.
	int get_action_state() const { return m_action_state; }
	Ref<ViewHolder> get_selected_holder() const { return m_selected; }
	bool is_dragging() const { return m_action_state == ACTION_STATE_DRAG && m_selected.is_valid(); }
	// True while the holder is being dragged/swiped or settling in a recover
	// animation: the RV excludes such holders from ItemAnimator move/add.
	bool is_occupied(const Ref<ViewHolder> &p_holder) const;
	void set_long_press_timeout(double p_ms);

	// Event/step hooks called from RecyclerView.
	bool on_press(const Ref<InputEventMouseButton> &p_event, RecyclerView *p_rv);
	bool on_motion(const Ref<InputEventMouseMotion> &p_event, RecyclerView *p_rv);
	bool on_release(const Ref<InputEventMouseButton> &p_event, RecyclerView *p_rv);
	// Re-pins the selected holder after a layout pass so a swap relayout (which
	// writes the holder's new slot) does not visibly tear it from the finger.
	void on_after_layout(RecyclerView *p_rv);
	// Drives the long-press timer, the edge auto-scroll and the recover
	// animations. Called every frame from the RV's _process.
	void step(double p_delta);

private:
	static constexpr float SLOP = 8.0f;
	static constexpr double DEFAULT_LONG_PRESS_TIMEOUT_S = 0.4;
	static constexpr float DEFAULT_SWIPE_ESCAPE_VELOCITY = 120.0f;
	static constexpr float DEFAULT_MAX_SWIPE_VELOCITY = 800.0f;
	static constexpr float EDGE_SCROLL_MAX_PX_S = 1200.0f;

	// The render position of the selected holder this frame: the layout slot
	// captured at grab time plus the clamped finger delta (Android's
	// mSelectedStart + mDx model).
	Vector2 target_render() const;

	void begin_drag(const Ref<ViewHolder> &p_holder);
	void select_for_swipe(const Ref<ViewHolder> &p_holder);
	void check_select_for_swipe(const Ref<InputEventMouseMotion> &p_event);
	void move_if_necessary();
	Vector<Ref<ViewHolder>> find_swap_targets(const Ref<ViewHolder> &p_selected);
	Ref<ViewHolder> choose_drop_target(const Ref<ViewHolder> &p_selected, const Vector<Ref<ViewHolder>> &p_targets, const Vector2 &p_cur);
	void update_dx_dy(const Vector2 &p_mouse);
	void re_pin();
	void on_release_internal();
	int swipe_if_necessary();
	int check_horizontal_swipe(int p_flags);
	int check_vertical_swipe(int p_flags);
	float get_velocity(bool p_horizontal) const;

	void start_recover_animation(const Ref<ViewHolder> &p_holder, const Vector2 &p_from, const Vector2 &p_to, double p_duration, int p_type, int p_swipe_dir, bool p_follow_layout);
	void clear_view(const Ref<ViewHolder> &p_holder);
	void cancel_long_press();
	void edge_scroll(double p_delta);
	bool in_recover_animation(const Ref<ViewHolder> &p_holder) const;

	Ref<ItemTouchHelperCallback> m_callback;
	RecyclerView *m_recycler_view = nullptr;

	int m_action_state = ACTION_STATE_IDLE;
	Ref<ViewHolder> m_selected;
	Vector2 m_selected_start; // layout slot (with insets) at grab time
	Vector2 m_initial_mouse;  // grab point (RV-local)
	Vector2 m_last_mouse;     // last motion position
	Vector2 m_dx;             // finger delta from m_initial_mouse, clamped by flags
	int m_selected_flags = 0; // directional flags for the current action state

	// Long-press timer.
	Ref<ViewHolder> m_press_holder;
	Vector2 m_press_pos;
	double m_press_elapsed = 0.0;
	double m_long_press_timeout_s = DEFAULT_LONG_PRESS_TIMEOUT_S;
	bool m_press_cancelled = true;
	// True between a press and its release (a gesture is in progress). Godot
	// also delivers motion events while hovering with no button down; swipe
	// selection is gated on this so a plain hover never starts a swipe.
	bool m_has_press = false;

	// Velocity sampling for the swipe-commit flick check.
	struct VelocitySample {
		float pos;
		double time_ms;
	};
	Vector<VelocitySample> m_samples_h;
	Vector<VelocitySample> m_samples_v;
	void push_sample(const Vector2 &p_mouse);

	// Self-drawn settle animations (drag drop / swipe out / swipe bounce-back).
	struct RecoverAnimation {
		Ref<ViewHolder> holder;
		Vector2 from;
		Vector2 to;
		double elapsed = 0.0;
		double duration = 0.0;
		int type = 0;
		int swipe_dir = 0;
		// Drag drops re-query the layout target each frame: the final slot of the
		// last swap only lands after the pending deferred layout runs.
		bool follow_layout = false;
	};
	Vector<RecoverAnimation> m_recover_animations;

	double m_elapsed_ms = 0.0;
};

} // namespace godot
