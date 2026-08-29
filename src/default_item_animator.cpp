#include "default_item_animator.h"

#include "recycler_view.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {
namespace {

float ease_decelerate(float p_t) {
	return 1.0f - (1.0f - p_t) * (1.0f - p_t);
}

void set_control_alpha(Control *p_control, float p_alpha) {
	Color c = p_control->get_modulate();
	c.a = p_alpha;
	p_control->set_modulate(c);
}

} // namespace

void DefaultItemAnimator::_bind_methods() {
}

void DefaultItemAnimator::animate_add(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	ItemAnimator::animate_add(p_holder, p_from, p_to);
	Control *control = p_holder->get_control();
	if (control != nullptr) {
		set_control_alpha(control, 0.0f);
	}
	m_fades.push_back({ p_holder, false, 0.0f });
}

void DefaultItemAnimator::animate_remove(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	ItemAnimator::animate_remove(p_holder, p_from, p_to);
	// Drop any move/change still running for this holder from a previous update.
	// After removal the holder's position is NO_POSITION, so a lingering move's
	// re-queried target becomes (0,0) and would drag the fading control onto the
	// first item, overlapping it.
	for (int i = m_moves.size() - 1; i >= 0; i--) {
		if (m_moves[i].holder == p_holder) {
			m_moves.remove_at(i);
		}
	}
	for (int i = m_changes.size() - 1; i >= 0; i--) {
		if (m_changes[i].holder == p_holder) {
			m_changes.remove_at(i);
		}
	}
	m_fades.push_back({ p_holder, true, 0.0f });
}

void DefaultItemAnimator::animate_move(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	ItemAnimator::animate_move(p_holder, p_from, p_to);
	// The layout already positioned the control at the post position; snap it
	// back to the start right away, or the frame before the first animate_step
	// would render the item at its end and then "restart" it (visible jitter).
	Control *control = p_holder->get_control();
	if (control != nullptr) {
		control->set_position(p_from.position);
	}
	// A rapid follow-up update supersedes the previous move for the same holder;
	// keeping both would fight over the control every frame.
	for (int i = 0; i < m_moves.size(); i++) {
		if (m_moves[i].holder == p_holder) {
			m_moves.remove_at(i);
			break;
		}
	}
	m_moves.push_back({ p_holder, p_from.position, 0.0f });
}

void DefaultItemAnimator::animate_change(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	ItemAnimator::animate_change(p_holder, p_from, p_to);
	// A rapid follow-up change supersedes the previous pulse for this holder.
	for (int i = 0; i < m_changes.size(); i++) {
		if (m_changes[i].holder == p_holder) {
			m_changes.remove_at(i);
			break;
		}
	}
	m_changes.push_back({ p_holder, false, 0.0f });
}

void DefaultItemAnimator::unmark_if_last(const Ref<ViewHolder> &p_holder) {
	for (int i = 0; i < m_moves.size(); i++) {
		if (m_moves[i].holder == p_holder) {
			return;
		}
	}
	for (int i = 0; i < m_fades.size(); i++) {
		if (m_fades[i].holder == p_holder) {
			return;
		}
	}
	for (int i = 0; i < m_changes.size(); i++) {
		if (m_changes[i].holder == p_holder) {
			return;
		}
	}
	unmark_animating(p_holder);
	if (m_recycler_view != nullptr) {
		// The holder's last animation finished: if it scrolled out of view
		// meanwhile, return it to the pool now rather than on the next layout.
		m_recycler_view->recycle_if_out_of_view(p_holder);
	}
}

void DefaultItemAnimator::cancel_holder(const Ref<ViewHolder> &p_holder) {
	// The holder scrolled out of view mid-animation: drop every queued/running
	// animation for it so the layout can recycle it now (see
	// LinearLayoutManager::on_layout_children).
	for (int i = m_moves.size() - 1; i >= 0; i--) {
		if (m_moves[i].holder == p_holder) {
			m_moves.remove_at(i);
		}
	}
	for (int i = m_fades.size() - 1; i >= 0; i--) {
		if (m_fades[i].holder == p_holder) {
			m_fades.remove_at(i);
		}
	}
	for (int i = m_changes.size() - 1; i >= 0; i--) {
		if (m_changes[i].holder == p_holder) {
			m_changes.remove_at(i);
		}
	}
	ItemAnimator::cancel_holder(p_holder);
}

void DefaultItemAnimator::animate_step(double p_delta) {
	for (int i = m_moves.size() - 1; i >= 0; i--) {
		MoveAnim &m = m_moves.write[i];
		m.elapsed += (float)p_delta;
		const float t = MIN(m.elapsed / MOVE_DURATION, 1.0f);
		Control *control = m.holder->get_control();
		if (control != nullptr && m_recycler_view != nullptr) {
			// Re-query the target each frame so the slide follows scrolls and
			// settles exactly on the layout position.
			const Vector2 to = m_recycler_view->get_layout_position(m.holder);
			control->set_position(m.from.lerp(to, ease_decelerate(t)));
		}
		if (t >= 1.0f) {
			const Ref<ViewHolder> holder = m.holder;
			m_moves.remove_at(i);
			unmark_if_last(holder);
		}
	}

	for (int i = m_fades.size() - 1; i >= 0; i--) {
		FadeAnim &f = m_fades.write[i];
		f.elapsed += (float)p_delta;
		const float duration = f.is_remove ? REMOVE_DURATION : ADD_DURATION;
		const float t = MIN(f.elapsed / duration, 1.0f);
		Control *control = f.holder->get_control();
		if (control != nullptr) {
			set_control_alpha(control, f.is_remove ? (1.0f - t) : t);
		}
		if (t >= 1.0f) {
			const Ref<ViewHolder> holder = f.holder;
			if (f.is_remove && m_recycler_view != nullptr) {
				m_recycler_view->recycle_removed(holder);
			}
			m_fades.remove_at(i);
			unmark_if_last(holder);
		}
	}

	for (int i = m_changes.size() - 1; i >= 0; i--) {
		FadeAnim &c = m_changes.write[i];
		c.elapsed += (float)p_delta;
		const float t = MIN(c.elapsed / CHANGE_DURATION, 1.0f);
		Control *control = c.holder->get_control();
		if (control != nullptr) {
			// Pulse: dip to 30% opacity at the midpoint, recover by the end.
			set_control_alpha(control, 1.0f - 0.7f * Math::sin(Math_PI * t));
		}
		if (t >= 1.0f) {
			const Ref<ViewHolder> holder = c.holder;
			if (control != nullptr) {
				set_control_alpha(control, 1.0f);
			}
			m_changes.remove_at(i);
			unmark_if_last(holder);
		}
	}
}

void DefaultItemAnimator::clear() {
	m_moves.clear();
	m_fades.clear();
	m_changes.clear();
	m_animating.clear();
}

} // namespace godot
