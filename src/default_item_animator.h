#pragma once

#include "item_animator.h"

#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Standard add/remove/move/change animations, driven frame-by-frame by the
// RecyclerView's _process via animate_step. Removed holders fade out and are
// recycled on completion; moving holders slide from their pre-update position
// to the layout's current target (so they follow scrolls).
class DefaultItemAnimator : public ItemAnimator {
	GDCLASS(DefaultItemAnimator, ItemAnimator)

protected:
	static void _bind_methods();

public:
	void animate_add(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) override;
	void animate_remove(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) override;
	void animate_move(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) override;
	void animate_change(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) override;
	void animate_step(double p_delta) override;
	void cancel_holder(const Ref<ViewHolder> &p_holder) override;
	// Drops every queued/running animation (teardown).
	void clear() override;

private:
	// Unmarks p_holder only if no other animation (move/fade/change) still
	// references it. A holder can animate in several lists at once (e.g. an add
	// fade while a move also slides it); unmarking on the first finish would let
	// the layout recycle it while the remaining animation still touches its
	// control (use-after-free once the control is released back to the pool).
	void unmark_if_last(const Ref<ViewHolder> &p_holder);

private:
	struct MoveAnim {
		Ref<ViewHolder> holder;
		Vector2 from;
		float elapsed = 0.0f;
	};
	struct FadeAnim {
		Ref<ViewHolder> holder;
		bool is_remove = false;
		float elapsed = 0.0f;
	};

	static constexpr float MOVE_DURATION = 0.3f;
	static constexpr float ADD_DURATION = 0.25f;
	static constexpr float REMOVE_DURATION = 0.25f;
	static constexpr float CHANGE_DURATION = 0.2f;

	Vector<MoveAnim> m_moves;
	Vector<FadeAnim> m_fades;
	Vector<FadeAnim> m_changes;
};

} // namespace godot
