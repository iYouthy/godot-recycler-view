#include "item_touch_helper.h"

#include "recycler_view.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// Direction flags live in ItemTouchHelperCallback::Direction (bound for GDScript);
// alias them here so the implementation reads as plain LEFT/RIGHT/UP/DOWN.
namespace {
constexpr int UP = ItemTouchHelperCallback::UP;
constexpr int DOWN = ItemTouchHelperCallback::DOWN;
constexpr int LEFT = ItemTouchHelperCallback::LEFT;
constexpr int RIGHT = ItemTouchHelperCallback::RIGHT;
constexpr int START = ItemTouchHelperCallback::START;
constexpr int END = ItemTouchHelperCallback::END;

// Android's RecoverAnimation easing: AccelerateDecelerateInterpolator.
float ease_in_out(float p_t) {
	return p_t < 0.5f ? 2.0f * p_t * p_t : 1.0f - (float)Math::pow(-2.0f * p_t + 2.0f, 2.0f) / 2.0f;
}
} // namespace

void ItemTouchHelper::_bind_methods() {
	ClassDB::bind_integer_constant(get_class_static(), "ActionState", "ACTION_STATE_IDLE", ACTION_STATE_IDLE);
	ClassDB::bind_integer_constant(get_class_static(), "ActionState", "ACTION_STATE_SWIPE", ACTION_STATE_SWIPE);
	ClassDB::bind_integer_constant(get_class_static(), "ActionState", "ACTION_STATE_DRAG", ACTION_STATE_DRAG);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "UP", ItemTouchHelperCallback::UP);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "DOWN", ItemTouchHelperCallback::DOWN);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "LEFT", ItemTouchHelperCallback::LEFT);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "RIGHT", ItemTouchHelperCallback::RIGHT);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "START", ItemTouchHelperCallback::START);
	ClassDB::bind_integer_constant(get_class_static(), "Direction", "END", ItemTouchHelperCallback::END);

	ClassDB::bind_static_method(get_class_static(), D_METHOD("make_movement_flags", "drag_flags", "swipe_flags"), &ItemTouchHelper::make_movement_flags);

	ClassDB::bind_method(D_METHOD("set_callback", "callback"), &ItemTouchHelper::set_callback);
	ClassDB::bind_method(D_METHOD("get_callback"), &ItemTouchHelper::get_callback);
	ClassDB::bind_method(D_METHOD("attach_to_recycler_view", "recycler_view"), &ItemTouchHelper::attach_to_recycler_view);
	ClassDB::bind_method(D_METHOD("detach"), &ItemTouchHelper::detach);
	ClassDB::bind_method(D_METHOD("get_action_state"), &ItemTouchHelper::get_action_state);
	ClassDB::bind_method(D_METHOD("get_selected_holder"), &ItemTouchHelper::get_selected_holder);
	ClassDB::bind_method(D_METHOD("is_dragging"), &ItemTouchHelper::is_dragging);
	ClassDB::bind_method(D_METHOD("is_occupied", "holder"), &ItemTouchHelper::is_occupied);
	ClassDB::bind_method(D_METHOD("set_long_press_timeout", "ms"), &ItemTouchHelper::set_long_press_timeout);
}

int ItemTouchHelper::make_movement_flags(int p_drag_flags, int p_swipe_flags) {
	const auto make_flag = [](int p_state, int p_dir) { return p_dir << (p_state * 8); };
	return make_flag(ACTION_STATE_IDLE, p_drag_flags | p_swipe_flags)
			| make_flag(ACTION_STATE_SWIPE, p_swipe_flags)
			| make_flag(ACTION_STATE_DRAG, p_drag_flags);
}

ItemTouchHelper::ItemTouchHelper() {}

ItemTouchHelper::~ItemTouchHelper() {}

void ItemTouchHelper::set_callback(const Ref<ItemTouchHelperCallback> &p_callback) {
	m_callback = p_callback;
}

void ItemTouchHelper::attach_to_recycler_view(RecyclerView *p_recycler_view) {
	if (m_recycler_view != nullptr && m_recycler_view != p_recycler_view) {
		m_recycler_view->set_item_touch_helper(Ref<ItemTouchHelper>());
	}
	m_recycler_view = p_recycler_view;
	if (m_recycler_view != nullptr) {
		m_recycler_view->set_item_touch_helper(Ref<ItemTouchHelper>(this));
	}
}

void ItemTouchHelper::detach() {
	if (m_recycler_view != nullptr) {
		m_recycler_view->set_item_touch_helper(Ref<ItemTouchHelper>());
	}
	m_recycler_view = nullptr;
}

void ItemTouchHelper::on_recycler_view_destroyed() {
	m_action_state = ACTION_STATE_IDLE;
	m_selected = Ref<ViewHolder>();
	m_recycler_view = nullptr;
	m_has_press = false;
	m_press_holder = Ref<ViewHolder>();
	m_recover_animations.clear();
}

bool ItemTouchHelper::is_occupied(const Ref<ViewHolder> &p_holder) const {
	if (m_action_state != ACTION_STATE_IDLE && m_selected == p_holder) {
		return true;
	}
	return in_recover_animation(p_holder);
}

bool ItemTouchHelper::in_recover_animation(const Ref<ViewHolder> &p_holder) const {
	for (int i = 0; i < m_recover_animations.size(); i++) {
		if (m_recover_animations[i].holder == p_holder) {
			return true;
		}
	}
	return false;
}

void ItemTouchHelper::set_long_press_timeout(double p_ms) {
	m_long_press_timeout_s = p_ms / 1000.0;
}

// --- Event hooks (called from RecyclerView::_gui_input, before scroll) ---

bool ItemTouchHelper::on_press(const Ref<InputEventMouseButton> &p_event, RecyclerView *p_rv) {
	if (m_recycler_view == nullptr || m_callback.is_null()) {
		return false;
	}
	const Vector2 pos = p_event->get_position();
	m_press_pos = pos;
	m_last_mouse = pos;
	m_has_press = true;
	m_press_holder = m_recycler_view->find_child_holder_at(pos);
	m_press_elapsed = 0.0;
	m_press_cancelled = m_press_holder.is_null();
	m_samples_h.clear();
	m_samples_v.clear();
	// Do not consume: the RV keeps its press handling; a long-press or swipe
	// later steals the gesture via cancel_drag().
	return false;
}

bool ItemTouchHelper::on_motion(const Ref<InputEventMouseMotion> &p_event, RecyclerView *p_rv) {
	if (m_recycler_view == nullptr || m_callback.is_null()) {
		return false;
	}
	m_last_mouse = p_event->get_position();
	if (m_action_state == ACTION_STATE_IDLE) {
		check_select_for_swipe(p_event);
		// Cancel a pending long-press once the finger moves beyond the slop
		// (the gesture belongs to the RV's scroll from here on).
		if (m_action_state == ACTION_STATE_IDLE && !m_press_cancelled
				&& m_last_mouse.distance_to(m_press_pos) > SLOP) {
			cancel_long_press();
		}
		return m_action_state != ACTION_STATE_IDLE;
	}
	// A release outside the window never arrives as a button event; the left
	// button coming up in the motion mask means the gesture was interrupted.
	if (!p_event->get_button_mask().has_flag(MouseButtonMask::MOUSE_BUTTON_MASK_LEFT)) {
		on_release_internal();
		return true;
	}
	push_sample(m_last_mouse);
	update_dx_dy(m_last_mouse);
	re_pin();
	if (m_action_state == ACTION_STATE_DRAG) {
		move_if_necessary();
	}
	return true;
}

bool ItemTouchHelper::on_release(const Ref<InputEventMouseButton> &p_event, RecyclerView *p_rv) {
	if (m_recycler_view == nullptr || m_callback.is_null()) {
		return false;
	}
	if (m_action_state == ACTION_STATE_IDLE) {
		cancel_long_press();
		m_has_press = false;
		return false;
	}
	on_release_internal();
	return true;
}

void ItemTouchHelper::on_release_internal() {
	m_has_press = false;
	if (m_selected.is_null()) {
		m_action_state = ACTION_STATE_IDLE;
		return;
	}
	int swipe_dir = 0;
	if (m_action_state == ACTION_STATE_SWIPE) {
		swipe_dir = swipe_if_necessary();
	}
	const int prev_state = m_action_state;
	const Ref<ViewHolder> holder = m_selected;
	const int anim_type = prev_state == ACTION_STATE_DRAG
			? ANIMATION_TYPE_DRAG
			: (swipe_dir > 0 ? ANIMATION_TYPE_SWIPE_SUCCESS : ANIMATION_TYPE_SWIPE_CANCEL);
	Control *control = holder->get_control();
	const Vector2 from = control != nullptr ? control->get_position() : Vector2();
	const Vector2 slot = m_recycler_view->get_layout_position(holder);
	Vector2 to = slot;
	if (prev_state == ACTION_STATE_DRAG) {
		// Settle to the final layout slot (already placed by the last swap).
	} else if (swipe_dir > 0) {
		// Slide a full viewport off-screen in the swipe direction.
		const Vector2 viewport = m_recycler_view->get_size();
		if (swipe_dir == LEFT || swipe_dir == START) {
			to = slot + Vector2(-viewport.x, 0.0f);
		} else if (swipe_dir == RIGHT || swipe_dir == END) {
			to = slot + Vector2(viewport.x, 0.0f);
		} else if (swipe_dir == UP) {
			to = slot + Vector2(0.0f, -viewport.y);
		} else {
			to = slot + Vector2(0.0f, viewport.y);
		}
	}
	const double duration = prev_state == ACTION_STATE_DRAG ? 0.2 : 0.25;
	m_action_state = ACTION_STATE_IDLE;
	m_selected = Ref<ViewHolder>();
	start_recover_animation(holder, from, to, duration, anim_type, swipe_dir, prev_state == ACTION_STATE_DRAG);
}

// --- Long-press drag selection ---

void ItemTouchHelper::begin_drag(const Ref<ViewHolder> &p_holder) {
	m_action_state = ACTION_STATE_DRAG;
	m_selected = p_holder;
	m_selected_start = m_recycler_view->get_layout_position(p_holder);
	m_initial_mouse = m_last_mouse;
	m_dx = Vector2();
	m_selected_flags = (m_callback->get_movement_flags(m_recycler_view, p_holder) & 0xFF0000) >> 16;
	if (p_holder.is_valid() && p_holder->get_control() != nullptr) {
		p_holder->get_control()->set_z_index(10);
	}
	// The RV marked the press as a drag; the gesture now belongs to us, so
	// free its scroll machinery (grabber chain, velocity, state) and consume
	// every subsequent event.
	m_recycler_view->cancel_drag();
	m_callback->on_selected_changed(p_holder, ACTION_STATE_DRAG);
}

void ItemTouchHelper::select_for_swipe(const Ref<ViewHolder> &p_holder) {
	m_action_state = ACTION_STATE_SWIPE;
	m_selected = p_holder;
	m_selected_start = m_recycler_view->get_layout_position(p_holder);
	// The swipe delta is measured from the original press so the item follows
	// the finger from the very first exceeding motion.
	m_initial_mouse = m_press_pos;
	m_dx = Vector2();
	m_selected_flags = (m_callback->get_movement_flags(m_recycler_view, p_holder) & 0xFF00) >> 8;
	if (p_holder.is_valid() && p_holder->get_control() != nullptr) {
		p_holder->get_control()->set_z_index(10);
	}
	m_recycler_view->cancel_drag();
	m_callback->on_selected_changed(p_holder, ACTION_STATE_SWIPE);
}

void ItemTouchHelper::check_select_for_swipe(const Ref<InputEventMouseMotion> &p_event) {
	if (m_selected.is_valid() || !m_callback->is_item_view_swipe_enabled() || m_recycler_view == nullptr) {
		return;
	}
	// A swipe can only begin from a press on an item. Godot delivers motion
	// events while merely hovering (no button), and m_press_pos would be stale
	// or zeroed, making a large fake delta that selects and then dismisses an
	// item without any click.
	if (!m_has_press) {
		return;
	}
	// Never hijack a gesture the RV is already scrolling.
	if (m_recycler_view->is_scroll_drag_active()) {
		return;
	}
	const Vector2 delta = m_last_mouse - m_press_pos;
	const float abs_dx = Math::abs(delta.x);
	const float abs_dy = Math::abs(delta.y);
	if (abs_dx < SLOP && abs_dy < SLOP) {
		return;
	}
	// If the dominant axis is one the layout scrolls, it's a scroll gesture.
	const bool dominant_horizontal = abs_dx > abs_dy;
	if (dominant_horizontal && m_recycler_view->get_layout()->can_scroll_horizontally()) {
		return;
	}
	if (!dominant_horizontal && m_recycler_view->get_layout()->can_scroll_vertically()) {
		return;
	}
	const Ref<ViewHolder> holder = m_recycler_view->find_child_holder_at(m_last_mouse);
	if (holder.is_null()) {
		return;
	}
	const int flags = m_callback->get_movement_flags(m_recycler_view, holder);
	const int swipe_flags = (flags & 0xFF00) >> 8;
	if (swipe_flags == 0) {
		return;
	}
	if (dominant_horizontal) {
		if (delta.x < 0 && (swipe_flags & LEFT) == 0) {
			return;
		}
		if (delta.x > 0 && (swipe_flags & RIGHT) == 0) {
			return;
		}
	} else {
		if (delta.y < 0 && (swipe_flags & UP) == 0) {
			return;
		}
		if (delta.y > 0 && (swipe_flags & DOWN) == 0) {
			return;
		}
	}
	cancel_long_press();
	select_for_swipe(holder);
	update_dx_dy(m_last_mouse);
	re_pin();
}

// --- Drag mechanics ---

Vector2 ItemTouchHelper::target_render() const {
	return m_selected_start + m_dx;
}

void ItemTouchHelper::update_dx_dy(const Vector2 &p_mouse) {
	Vector2 delta = p_mouse - m_initial_mouse;
	if ((m_selected_flags & LEFT) == 0) {
		delta.x = Math::max(0.0f, delta.x);
	}
	if ((m_selected_flags & RIGHT) == 0) {
		delta.x = Math::min(0.0f, delta.x);
	}
	if ((m_selected_flags & UP) == 0) {
		delta.y = Math::max(0.0f, delta.y);
	}
	if ((m_selected_flags & DOWN) == 0) {
		delta.y = Math::min(0.0f, delta.y);
	}
	m_dx = delta;
}

void ItemTouchHelper::re_pin() {
	if (m_selected.is_valid() && m_selected->get_control() != nullptr) {
		m_selected->get_control()->set_position(target_render());
	}
}

void ItemTouchHelper::on_after_layout(RecyclerView *p_rv) {
	// A layout pass writes every holder's slot. Re-pin the selected holder to
	// the finger, and any recover animation to its interpolated position, so a
	// relayout never tears a gesture visually.
	if (m_action_state != ACTION_STATE_IDLE && m_selected.is_valid()) {
		re_pin();
	}
	for (int i = 0; i < m_recover_animations.size(); i++) {
		RecoverAnimation &anim = m_recover_animations.write[i];
		const float t = anim.duration > 0.0 ? CLAMP((float)(anim.elapsed / anim.duration), 0.0f, 1.0f) : 1.0f;
		const Vector2 target = anim.follow_layout ? m_recycler_view->get_layout_position(anim.holder) : anim.to;
		if (anim.holder.is_valid() && anim.holder->get_control() != nullptr) {
			anim.holder->get_control()->set_position(anim.from.lerp(target, ease_in_out(t)));
		}
	}
}

void ItemTouchHelper::move_if_necessary() {
	if (m_selected.is_null() || m_recycler_view == nullptr) {
		return;
	}
	// A swap already deferred a layout this frame: slot positions are stale
	// until it runs, and swapping again would churn the same pair (mirrors
	// Android's isLayoutRequested() guard in moveIfNecessary).
	if (m_recycler_view->is_layout_requested()) {
		return;
	}
	const float threshold = m_callback->get_move_threshold(m_selected);
	const Vector2 slot = m_recycler_view->get_layout_position(m_selected);
	const Vector2 render = target_render();
	const float w = m_selected->get_control() != nullptr ? m_selected->get_control()->get_size().x : 0.0f;
	const float h = m_selected->get_control() != nullptr ? m_selected->get_control()->get_size().y : 0.0f;
	// Not yet past the midpoint of the current slot: keep dragging.
	if (Math::abs(render.y - slot.y) < h * threshold && Math::abs(render.x - slot.x) < w * threshold) {
		return;
	}
	const Vector<Ref<ViewHolder>> targets = find_swap_targets(m_selected);
	if (targets.size() == 0) {
		return;
	}
	const Ref<ViewHolder> target = choose_drop_target(m_selected, targets, render);
	if (target.is_null()) {
		return;
	}
	m_callback->on_move(m_recycler_view, m_selected, target);
}

Vector<Ref<ViewHolder>> ItemTouchHelper::find_swap_targets(const Ref<ViewHolder> &p_selected) {
	Vector<Ref<ViewHolder>> targets;
	Vector<int> distances;
	const float margin = m_callback->get_bounding_box_margin();
	const Vector2 render = target_render();
	const float w = p_selected->get_control() != nullptr ? p_selected->get_control()->get_size().x : 0.0f;
	const float h = p_selected->get_control() != nullptr ? p_selected->get_control()->get_size().y : 0.0f;
	const float left = render.x - margin;
	const float top = render.y - margin;
	const float right = left + w + 2.0f * margin;
	const float bottom = top + h + 2.0f * margin;
	const float center_x = (left + right) / 2.0f;
	const float center_y = (top + bottom) / 2.0f;
	const int count = m_recycler_view->get_child_holder_count();
	for (int i = 0; i < count; i++) {
		Ref<ViewHolder> other = m_recycler_view->get_child_holder_at(i);
		if (other == p_selected) {
			continue;
		}
		const Vector2 slot = m_recycler_view->get_layout_position(other);
		const float ow = other->get_control() != nullptr ? other->get_control()->get_size().x : 0.0f;
		const float oh = other->get_control() != nullptr ? other->get_control()->get_size().y : 0.0f;
		if (slot.y + oh < top || slot.y > bottom || slot.x + ow < left || slot.x > right) {
			continue;
		}
		if (!m_callback->can_drop_over(p_selected, other)) {
			continue;
		}
		const float dx = Math::abs(center_x - (slot.x + ow / 2.0f));
		const float dy = Math::abs(center_y - (slot.y + oh / 2.0f));
		const int dist = (int)(dx * dx + dy * dy);
		int pos = 0;
		for (int j = 0; j < targets.size(); j++) {
			if (dist > distances[j]) {
				pos++;
			} else {
				break;
			}
		}
		targets.insert(pos, other);
		distances.insert(pos, dist);
	}
	return targets;
}

Ref<ViewHolder> ItemTouchHelper::choose_drop_target(const Ref<ViewHolder> &p_selected,
		const Vector<Ref<ViewHolder>> &p_targets, const Vector2 &p_cur) {
	const float w = p_selected->get_control() != nullptr ? p_selected->get_control()->get_size().x : 0.0f;
	const float h = p_selected->get_control() != nullptr ? p_selected->get_control()->get_size().y : 0.0f;
	const Vector2 slot = m_recycler_view->get_layout_position(p_selected);
	const float dx = p_cur.x - slot.x;
	const float dy = p_cur.y - slot.y;
	Ref<ViewHolder> winner;
	int winner_score = -1;
	for (int i = 0; i < p_targets.size(); i++) {
		const Ref<ViewHolder> target = p_targets[i];
		const Vector2 t_slot = m_recycler_view->get_layout_position(target);
		const float tw = target->get_control() != nullptr ? target->get_control()->get_size().x : 0.0f;
		const float th = target->get_control() != nullptr ? target->get_control()->get_size().y : 0.0f;
		// Only swap when the dragged item has actually crossed the target in
		// the movement direction, favoring the furthest crossed target.
		if (dx > 0) {
			const float diff = (t_slot.x + tw) - (p_cur.x + w);
			if (diff < 0 && (t_slot.x + tw) > (slot.x + w)) {
				const int score = (int)Math::abs(diff);
				if (score > winner_score) {
					winner_score = score;
					winner = target;
				}
			}
		}
		if (dx < 0) {
			const float diff = t_slot.x - p_cur.x;
			if (diff > 0 && t_slot.x < slot.x) {
				const int score = (int)Math::abs(diff);
				if (score > winner_score) {
					winner_score = score;
					winner = target;
				}
			}
		}
		if (dy < 0) {
			const float diff = t_slot.y - p_cur.y;
			if (diff > 0 && t_slot.y < slot.y) {
				const int score = (int)Math::abs(diff);
				if (score > winner_score) {
					winner_score = score;
					winner = target;
				}
			}
		}
		if (dy > 0) {
			const float diff = (t_slot.y + th) - (p_cur.y + h);
			if (diff < 0 && (t_slot.y + th) > (slot.y + h)) {
				const int score = (int)Math::abs(diff);
				if (score > winner_score) {
					winner_score = score;
					winner = target;
				}
			}
		}
	}
	return winner;
}

// --- Swipe commit ---

void ItemTouchHelper::push_sample(const Vector2 &p_mouse) {
	m_samples_h.push_back({ p_mouse.x, m_elapsed_ms });
	m_samples_v.push_back({ p_mouse.y, m_elapsed_ms });
	while (m_samples_h.size() > 5) {
		m_samples_h.remove_at(0);
	}
	while (m_samples_v.size() > 5) {
		m_samples_v.remove_at(0);
	}
}

float ItemTouchHelper::get_velocity(bool p_horizontal) const {
	const Vector<VelocitySample> &samples = p_horizontal ? m_samples_h : m_samples_v;
	if (samples.size() < 2) {
		return 0.0f;
	}
	const float dp = samples[samples.size() - 1].pos - samples[0].pos;
	const double dt = samples[samples.size() - 1].time_ms - samples[0].time_ms;
	if (dt <= 0.0) {
		return 0.0f;
	}
	return dp / (float)(dt / 1000.0);
}

int ItemTouchHelper::swipe_if_necessary() {
	if (m_action_state != ACTION_STATE_SWIPE || m_selected.is_null()) {
		return 0;
	}
	const int flags = (m_callback->get_movement_flags(m_recycler_view, m_selected) & 0xFF00) >> 8;
	if (flags == 0) {
		return 0;
	}
	int swipe_dir;
	if (Math::abs(m_dx.x) > Math::abs(m_dx.y)) {
		if ((swipe_dir = check_horizontal_swipe(flags)) > 0) {
			return swipe_dir;
		}
		if ((swipe_dir = check_vertical_swipe(flags)) > 0) {
			return swipe_dir;
		}
	} else {
		if ((swipe_dir = check_vertical_swipe(flags)) > 0) {
			return swipe_dir;
		}
		if ((swipe_dir = check_horizontal_swipe(flags)) > 0) {
			return swipe_dir;
		}
	}
	return 0;
}

int ItemTouchHelper::check_horizontal_swipe(int p_flags) {
	if ((p_flags & (LEFT | RIGHT)) == 0) {
		return 0;
	}
	const int dir_flag = m_dx.x > 0 ? RIGHT : LEFT;
	const float xv = get_velocity(true);
	const float yv = get_velocity(false);
	const int vel_dir = xv > 0 ? RIGHT : LEFT;
	const float escape = m_callback->get_swipe_escape_velocity(DEFAULT_SWIPE_ESCAPE_VELOCITY);
	// Flick: velocity in the movement direction past the escape threshold and
	// dominant over the cross axis.
	if ((vel_dir & p_flags) != 0 && vel_dir == dir_flag
			&& Math::abs(xv) >= escape && Math::abs(xv) > Math::abs(yv)) {
		return vel_dir;
	}
	const float threshold = m_recycler_view->get_size().x * m_callback->get_swipe_threshold(m_selected);
	if ((p_flags & dir_flag) != 0 && Math::abs(m_dx.x) > threshold) {
		return dir_flag;
	}
	return 0;
}

int ItemTouchHelper::check_vertical_swipe(int p_flags) {
	if ((p_flags & (UP | DOWN)) == 0) {
		return 0;
	}
	const int dir_flag = m_dx.y > 0 ? DOWN : UP;
	const float xv = get_velocity(true);
	const float yv = get_velocity(false);
	const int vel_dir = yv > 0 ? DOWN : UP;
	const float escape = m_callback->get_swipe_escape_velocity(DEFAULT_SWIPE_ESCAPE_VELOCITY);
	if ((vel_dir & p_flags) != 0 && vel_dir == dir_flag
			&& Math::abs(yv) >= escape && Math::abs(yv) > Math::abs(xv)) {
		return vel_dir;
	}
	const float threshold = m_recycler_view->get_size().y * m_callback->get_swipe_threshold(m_selected);
	if ((p_flags & dir_flag) != 0 && Math::abs(m_dx.y) > threshold) {
		return dir_flag;
	}
	return 0;
}

// --- Recover animations ---

void ItemTouchHelper::start_recover_animation(const Ref<ViewHolder> &p_holder,
		const Vector2 &p_from, const Vector2 &p_to, double p_duration, int p_type, int p_swipe_dir, bool p_follow_layout) {
	RecoverAnimation anim;
	anim.holder = p_holder;
	anim.from = p_from;
	anim.to = p_to;
	anim.duration = p_duration;
	anim.type = p_type;
	anim.swipe_dir = p_swipe_dir;
	anim.follow_layout = p_follow_layout;
	m_recover_animations.push_back(anim);
}

void ItemTouchHelper::clear_view(const Ref<ViewHolder> &p_holder) {
	if (p_holder.is_valid() && p_holder->get_control() != nullptr) {
		p_holder->get_control()->set_z_index(0);
	}
	m_selected = Ref<ViewHolder>();
	m_action_state = ACTION_STATE_IDLE;
	if (m_callback.is_valid()) {
		m_callback->clear_view(p_holder);
	}
}

void ItemTouchHelper::cancel_long_press() {
	m_press_holder = Ref<ViewHolder>();
	m_press_cancelled = true;
	m_press_elapsed = 0.0;
}

void ItemTouchHelper::edge_scroll(double p_delta) {
	if (m_selected.is_null() || m_recycler_view == nullptr) {
		return;
	}
	const bool can_v = m_recycler_view->get_layout()->can_scroll_vertically();
	const bool can_h = m_recycler_view->get_layout()->can_scroll_horizontally();
	if (!can_v && !can_h) {
		return;
	}
	const Vector2 render = target_render();
	const Vector2 viewport = m_recycler_view->get_viewport_size();
	const float w = m_selected->get_control() != nullptr ? m_selected->get_control()->get_size().x : 0.0f;
	const float h = m_selected->get_control() != nullptr ? m_selected->get_control()->get_size().y : 0.0f;
	float rate = 0.0f;
	int dir = 0;
	if (can_v) {
		if (render.y < 0.0f) {
			rate = CLAMP(-render.y / viewport.y, 0.0f, 1.0f);
			dir = -1;
		} else if (render.y + h > viewport.y) {
			rate = CLAMP((render.y + h - viewport.y) / viewport.y, 0.0f, 1.0f);
			dir = 1;
		}
	} else if (can_h) {
		if (render.x < 0.0f) {
			rate = CLAMP(-render.x / viewport.x, 0.0f, 1.0f);
			dir = -1;
		} else if (render.x + w > viewport.x) {
			rate = CLAMP((render.x + w - viewport.x) / viewport.x, 0.0f, 1.0f);
			dir = 1;
		}
	}
	if (dir == 0) {
		return;
	}
	const int amount = (int)(rate * EDGE_SCROLL_MAX_PX_S * (float)p_delta);
	if (can_v) {
		m_recycler_view->scroll_vertically(dir * amount);
	} else {
		m_recycler_view->scroll_horizontally(dir * amount);
	}
}

void ItemTouchHelper::step(double p_delta) {
	if (m_recycler_view == nullptr) {
		return;
	}
	m_elapsed_ms += p_delta * 1000.0;

	// Long-press timer.
	if (m_action_state == ACTION_STATE_IDLE && !m_press_cancelled && m_press_holder.is_valid()) {
		m_press_elapsed += p_delta;
		if (m_press_elapsed >= m_long_press_timeout_s) {
			const Ref<ViewHolder> holder = m_press_holder;
			m_press_holder = Ref<ViewHolder>();
			m_press_cancelled = true;
			if (m_callback->has_drag_flag(m_recycler_view, holder) && m_callback->is_long_press_drag_enabled()) {
				begin_drag(holder);
			}
		}
	}

	if (m_action_state == ACTION_STATE_DRAG) {
		edge_scroll(p_delta);
	}

	for (int i = m_recover_animations.size() - 1; i >= 0; i--) {
		RecoverAnimation &anim = m_recover_animations.write[i];
		anim.elapsed += p_delta;
		const float t = anim.duration > 0.0 ? (float)(anim.elapsed / anim.duration) : 1.0f;
		const Vector2 target = anim.follow_layout ? m_recycler_view->get_layout_position(anim.holder) : anim.to;
		if (anim.holder.is_valid() && anim.holder->get_control() != nullptr) {
			anim.holder->get_control()->set_position(anim.from.lerp(target, ease_in_out(CLAMP(t, 0.0f, 1.0f))));
		}
		if (t >= 1.0f) {
			const int type = anim.type;
			const int swipe_dir = anim.swipe_dir;
			const Ref<ViewHolder> holder = anim.holder;
			m_recover_animations.remove_at(i);
			clear_view(holder);
			if (type == ANIMATION_TYPE_SWIPE_SUCCESS) {
				// The item slid off-screen; hand off to the callback which is
				// expected to remove it from the data and notify.
				m_callback->on_swiped(holder, swipe_dir);
			}
		}
	}
}

} // namespace godot
