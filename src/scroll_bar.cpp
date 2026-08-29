#include "scroll_bar.h"

#include "layout_manager.h"
#include "recycler_view.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {

void RecyclerViewScrollBar::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_recycler_view", "recycler_view"), &RecyclerViewScrollBar::set_recycler_view);
	ClassDB::bind_method(D_METHOD("get_recycler_view"), &RecyclerViewScrollBar::get_recycler_view);
	ClassDB::bind_method(D_METHOD("set_axis", "axis"), &RecyclerViewScrollBar::set_axis);
	ClassDB::bind_method(D_METHOD("get_axis"), &RecyclerViewScrollBar::get_axis);
	ClassDB::bind_method(D_METHOD("set_thickness", "thickness"), &RecyclerViewScrollBar::set_thickness);
	ClassDB::bind_method(D_METHOD("get_thickness"), &RecyclerViewScrollBar::get_thickness);
	ClassDB::bind_method(D_METHOD("set_auto_hide", "enabled"), &RecyclerViewScrollBar::set_auto_hide);
	ClassDB::bind_method(D_METHOD("get_auto_hide"), &RecyclerViewScrollBar::get_auto_hide);
	ClassDB::bind_method(D_METHOD("set_hide_delay", "delay"), &RecyclerViewScrollBar::set_hide_delay);
	ClassDB::bind_method(D_METHOD("get_hide_delay"), &RecyclerViewScrollBar::get_hide_delay);
	ClassDB::bind_method(D_METHOD("on_scroll_changed"), &RecyclerViewScrollBar::on_scroll_changed);
	ClassDB::bind_method(D_METHOD("get_offset"), &RecyclerViewScrollBar::get_offset);
	ClassDB::bind_method(D_METHOD("get_viewport_size"), &RecyclerViewScrollBar::get_viewport_size);
	ClassDB::bind_method(D_METHOD("get_content_size"), &RecyclerViewScrollBar::get_content_size);
	ClassDB::bind_method(D_METHOD("bind_to", "recycler_view"), &RecyclerViewScrollBar::bind_to);
	ClassDB::bind_method(D_METHOD("unbind"), &RecyclerViewScrollBar::unbind);
	GDVIRTUAL_BIND(_on_scroll_changed);

	ClassDB::bind_integer_constant(get_class_static(), "Axis", "SCROLL_BAR_VERTICAL", SCROLL_BAR_VERTICAL);
	ClassDB::bind_integer_constant(get_class_static(), "Axis", "SCROLL_BAR_HORIZONTAL", SCROLL_BAR_HORIZONTAL);
}

void RecyclerViewScrollBar::set_recycler_view(RecyclerView *p_rv) {
	m_recycler_view = p_rv;
}

void RecyclerViewScrollBar::set_axis(int p_axis) {
	m_axis = p_axis;
	queue_redraw();
}

void RecyclerViewScrollBar::set_thickness(float p_thickness) {
	m_thickness = p_thickness;
	if (m_recycler_view != nullptr) {
		// Re-pin so the wider/thinner bar re-sizes immediately (custom bars set
		// the thickness from _init/_ready, before or after bind).
		apply_anchors();
	}
	queue_redraw();
}

void RecyclerViewScrollBar::set_auto_hide(bool p_enabled) {
	m_auto_hide = p_enabled;
}

void RecyclerViewScrollBar::set_hide_delay(float p_delay) {
	m_hide_delay = p_delay;
}

void RecyclerViewScrollBar::on_scroll_changed() {
	if (GDVIRTUAL_CALL(_on_scroll_changed)) {
		return;
	}
	queue_redraw();
}

int RecyclerViewScrollBar::get_offset() const {
	if (m_recycler_view == nullptr) {
		return 0;
	}
	int raw = m_axis == SCROLL_BAR_VERTICAL
			? m_recycler_view->get_scroll_offset()
			: m_recycler_view->get_scroll_offset_horizontal();
	// reverse_layout keeps the raw offset space (0 = content start) but flips the
	// content->screen mapping, so report the offset measured from the content end
	// instead: the thumb then tracks the visible region (offset 0 = content start
	// on screen shows the bottom, thumb at the bottom).
	if (m_recycler_view->get_layout().is_valid() && m_recycler_view->get_layout()->is_reverse_layout()) {
		raw = MAX(0, get_content_size() - get_viewport_size()) - raw;
	}
	return raw;
}

int RecyclerViewScrollBar::get_viewport_size() const {
	if (m_recycler_view == nullptr) {
		return 0;
	}
	const Vector2 viewport = m_recycler_view->get_viewport_size();
	return m_axis == SCROLL_BAR_VERTICAL ? (int)viewport.y : (int)viewport.x;
}

int RecyclerViewScrollBar::get_content_size() const {
	if (m_recycler_view == nullptr || m_recycler_view->get_layout().is_null()) {
		return 0;
	}
	return m_recycler_view->get_layout()->get_content_size(m_recycler_view);
}

void RecyclerViewScrollBar::bind_to(RecyclerView *p_rv) {
	m_recycler_view = p_rv;
	if (m_recycler_view != nullptr && m_recycler_view->get_layout().is_valid()
			&& m_recycler_view->get_layout()->can_scroll_horizontally()) {
		set_axis(SCROLL_BAR_HORIZONTAL);
	} else {
		set_axis(SCROLL_BAR_VERTICAL);
	}
	apply_anchors();
}

void RecyclerViewScrollBar::apply_anchors() {
	// Keep the bar above the item views and pin it to the trailing edge.
	set_z_index(100);
	if (m_axis == SCROLL_BAR_VERTICAL) {
		set_anchor(Side::SIDE_LEFT, 1.0f);
		set_anchor(Side::SIDE_RIGHT, 1.0f);
		set_anchor(Side::SIDE_TOP, 0.0f);
		set_anchor(Side::SIDE_BOTTOM, 1.0f);
		set_offset(Side::SIDE_LEFT, -m_thickness);
		set_offset(Side::SIDE_RIGHT, 0.0f);
		set_offset(Side::SIDE_TOP, 0.0f);
		set_offset(Side::SIDE_BOTTOM, 0.0f);
	} else {
		set_anchor(Side::SIDE_LEFT, 0.0f);
		set_anchor(Side::SIDE_RIGHT, 1.0f);
		set_anchor(Side::SIDE_TOP, 1.0f);
		set_anchor(Side::SIDE_BOTTOM, 1.0f);
		set_offset(Side::SIDE_LEFT, 0.0f);
		set_offset(Side::SIDE_RIGHT, 0.0f);
		set_offset(Side::SIDE_TOP, -m_thickness);
		set_offset(Side::SIDE_BOTTOM, 0.0f);
	}
}

void RecyclerViewScrollBar::unbind() {
	m_recycler_view = nullptr;
}

// ---------------------------------------------------------------------------
// DefaultScrollBar.

void DefaultScrollBar::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_track_color", "color"), &DefaultScrollBar::set_track_color);
	ClassDB::bind_method(D_METHOD("get_track_color"), &DefaultScrollBar::get_track_color);
	ClassDB::bind_method(D_METHOD("set_thumb_color", "color"), &DefaultScrollBar::set_thumb_color);
	ClassDB::bind_method(D_METHOD("get_thumb_color"), &DefaultScrollBar::get_thumb_color);
	ClassDB::bind_method(D_METHOD("set_corner_radius", "radius"), &DefaultScrollBar::set_corner_radius);
	ClassDB::bind_method(D_METHOD("get_corner_radius"), &DefaultScrollBar::get_corner_radius);
	ClassDB::bind_method(D_METHOD("set_auto_hide", "auto_hide"), &DefaultScrollBar::set_auto_hide);
	ClassDB::bind_method(D_METHOD("get_thumb_rect"), &DefaultScrollBar::get_thumb_rect);

	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "track_color"), "set_track_color", "get_track_color");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "thumb_color"), "set_thumb_color", "get_thumb_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "corner_radius"), "set_corner_radius", "get_corner_radius");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_hide"), "set_auto_hide", "get_auto_hide");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hide_delay"), "set_hide_delay", "get_hide_delay");
}

DefaultScrollBar::DefaultScrollBar() {
	set_mouse_filter(MOUSE_FILTER_IGNORE);
	set_process(true);
	// Inherit the built-in ScrollBar theme items (grabber / grabber_highlight /
	// scroll / scroll_focus) as the fallback chain. The current Godot default
	// theme's ScrollBar is a flat red box, so a classic grey rounded style is
	// installed as the default below; add_theme_stylebox_override() by the user
	// replaces these, and Theme resources still resolve through the ScrollBar
	// variation once the defaults are overridden/removed.
	set_theme_type_variation("ScrollBar");
	Ref<StyleBoxFlat> track;
	track.instantiate();
	track->set_bg_color(Color(0.0f, 0.0f, 0.0f, 0.1f));
	add_theme_stylebox_override("scroll", track);
	Ref<StyleBoxFlat> thumb;
	thumb.instantiate();
	thumb->set_bg_color(Color(0.5f, 0.5f, 0.5f, 0.6f));
	thumb->set_corner_radius_all(4.0f);
	add_theme_stylebox_override("grabber", thumb);
	Ref<StyleBoxFlat> thumb_hl;
	thumb_hl.instantiate();
	thumb_hl->set_bg_color(Color(0.6f, 0.6f, 0.6f, 0.7f));
	thumb_hl->set_corner_radius_all(4.0f);
	add_theme_stylebox_override("grabber_highlight", thumb_hl);
}

void DefaultScrollBar::set_track_color(const Color &p_color) {
	m_track_color = p_color;
	queue_redraw();
}

void DefaultScrollBar::set_thumb_color(const Color &p_color) {
	m_thumb_color = p_color;
	queue_redraw();
}

void DefaultScrollBar::set_corner_radius(float p_radius) {
	m_corner_radius = p_radius;
	queue_redraw();
}

void DefaultScrollBar::set_auto_hide(bool p_enabled) {
	RecyclerViewScrollBar::set_auto_hide(p_enabled);
	m_target_alpha = p_enabled ? m_target_alpha : 1.0f;
	queue_redraw();
}

void DefaultScrollBar::on_scroll_changed() {
	const int offset = get_offset();
	if (offset != m_last_offset) {
		m_last_offset = offset;
		m_idle_time = 0.0;
	}
	RecyclerViewScrollBar::on_scroll_changed();
}

Rect2 DefaultScrollBar::get_thumb_rect() const {
	const float track_len = m_axis == SCROLL_BAR_VERTICAL ? get_size().y : get_size().x;
	const int viewport = get_viewport_size();
	const int content = get_content_size();
	if (content <= viewport || track_len <= 0.0f) {
		return Rect2();
	}
	const float ratio = (float)viewport / content;
	float thumb_len = track_len * ratio;
	if (thumb_len < MIN_THUMB) {
		thumb_len = MIN_THUMB;
	}
	const int max_offset = content - viewport;
	const float t = max_offset > 0 ? (float)get_offset() / max_offset : 0.0f;
	const float thumb_off = (track_len - thumb_len) * t;
	if (m_axis == SCROLL_BAR_VERTICAL) {
		return Rect2(0.0f, thumb_off, get_size().x, thumb_len);
	}
	return Rect2(thumb_off, 0.0f, thumb_len, get_size().y);
}

void DefaultScrollBar::_draw() {
	// Godot's StyleBox theme system, mirroring the built-in ScrollBar theme
	// items and its self-managed states:
	//   "scroll" / "scroll_focus"      = track (focus state)
	//   "grabber"                      = thumb normal
	//   "grabber_highlight"            = thumb hovered or pressed
	// Users style it with add_theme_stylebox_override() / a Theme resource; the
	// color properties below are only a fallback when no theme provides one.
	const Rect2 track_rect(0.0f, 0.0f, get_size().x, get_size().y);
	const Ref<StyleBox> track_sb = get_theme_stylebox(has_focus() ? StringName("scroll_focus") : StringName("scroll"));
	if (track_sb.is_valid()) {
		draw_style_box(track_sb, track_rect);
	} else {
		Ref<StyleBoxFlat> flat;
		flat.instantiate();
		flat->set_bg_color(m_track_color);
		flat->set_corner_radius_all(m_corner_radius);
		draw_style_box(flat, track_rect);
	}
	const Rect2 thumb = get_thumb_rect();
	if (thumb.size.x > 0.0f && thumb.size.y > 0.0f) {
		const StringName sb_name = (m_hovered || m_dragging) ? StringName("grabber_highlight") : StringName("grabber");
		const Ref<StyleBox> thumb_sb = get_theme_stylebox(sb_name);
		if (thumb_sb.is_valid()) {
			draw_style_box(thumb_sb, thumb);
		} else {
			Ref<StyleBoxFlat> flat;
			flat.instantiate();
			flat->set_bg_color(m_thumb_color);
			flat->set_corner_radius_all(m_corner_radius);
			draw_style_box(flat, thumb);
		}
	}
}

void DefaultScrollBar::_gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
		if (mb->is_pressed()) {
			const Rect2 thumb = get_thumb_rect();
			const Vector2 pos = mb->get_position();
			const float along = m_axis == SCROLL_BAR_VERTICAL ? pos.y : pos.x;
			if (thumb.has_point(pos)) {
				m_dragging = true;
				m_last_along = along;
			} else {
				// Click on the track: jump the thumb center to the click.
				const float half = (m_axis == SCROLL_BAR_VERTICAL ? thumb.size.y : thumb.size.x) * 0.5f;
				scroll_to_pos(along - half);
			}
			accept_event();
		} else if (m_dragging) {
			m_dragging = false;
			accept_event();
		}
		return;
	}
	const Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid() && m_dragging) {
		// A motion without the left button means the release never reached us
		// (Godot routed it to another control); end the drag so the bar stops
		// following the mouse.
		if (!mm->get_button_mask().has_flag(MouseButtonMask::MOUSE_BUTTON_MASK_LEFT)) {
			m_dragging = false;
			return;
		}
		const Vector2 pos = mm->get_position();
		const float along = m_axis == SCROLL_BAR_VERTICAL ? pos.y : pos.x;
		const float diff = along - m_last_along;
		m_last_along = along;
		if (diff != 0.0f) {
			scroll_by_delta(diff);
		}
		accept_event();
	}
}

void DefaultScrollBar::scroll_to_pos(float p_thumb_start) {
	if (m_recycler_view == nullptr) {
		return;
	}
	const float track_len = m_axis == SCROLL_BAR_VERTICAL ? get_size().y : get_size().x;
	const Rect2 thumb = get_thumb_rect();
	const int viewport = get_viewport_size();
	const int content = get_content_size();
	const int max_offset = content - viewport;
	if (max_offset <= 0) {
		return;
	}
	const float thumb_len = m_axis == SCROLL_BAR_VERTICAL ? thumb.size.y : thumb.size.x;
	const float travel = track_len - thumb_len;
	if (travel <= 0.0f) {
		return;
	}
	const float t = CLAMP(p_thumb_start / travel, 0.0f, 1.0f);
	int target = (int)(t * max_offset);
	// Mirror get_offset: the thumb coordinates live in the "from content end"
	// space, translate back to the raw scroll offset before applying.
	if (m_recycler_view->get_layout().is_valid() && m_recycler_view->get_layout()->is_reverse_layout()) {
		target = max_offset - target;
	}
	if (m_axis == SCROLL_BAR_VERTICAL) {
		m_recycler_view->set_scroll_offset(target);
	} else {
		m_recycler_view->set_scroll_offset_horizontal(target);
	}
}

void DefaultScrollBar::scroll_by_delta(float p_delta) {
	if (m_recycler_view == nullptr) {
		return;
	}
	const float track_len = m_axis == SCROLL_BAR_VERTICAL ? get_size().y : get_size().x;
	const Rect2 thumb = get_thumb_rect();
	const float thumb_len = m_axis == SCROLL_BAR_VERTICAL ? thumb.size.y : thumb.size.x;
	const float travel = track_len - thumb_len;
	if (travel <= 0.0f) {
		return;
	}
	// Android's handleScrollBarDragging semantics: each motion event advances the
	// thumb by the mouse delta since the previous event. The viewport then only
	// shifts a fraction of its size per frame, so the reuse chain (position cache
	// + pool) absorbs it and no fresh holders are fabricated while dragging.
	const float start = m_axis == SCROLL_BAR_VERTICAL ? thumb.position.y : thumb.position.x;
	scroll_to_pos(start + p_delta);
}

void DefaultScrollBar::_process(double p_delta) {
	// While dragging, keep following the mouse even outside the window: Godot
	// stops sending motion events once the cursor leaves the window, so the
	// drag would otherwise stall. Polling the system mouse position keeps the
	// thumb pinned to the cursor (clamped to the content bounds).
	if (m_dragging && get_viewport() != nullptr && Input::get_singleton() != nullptr) {
		if (Input::get_singleton()->is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT)) {
			const Vector2 global = get_viewport()->get_mouse_position();
			const float along = m_axis == SCROLL_BAR_VERTICAL
					? global.y - get_global_position().y
					: global.x - get_global_position().x;
			const float diff = along - m_last_along;
			m_last_along = along;
			if (diff != 0.0f) {
				scroll_by_delta(diff);
			}
		} else {
			m_dragging = false;
		}
	}
	// Cursor inside the RV: moving it resets the idle timer so the bar stays
	// visible; only cursor stillness past the hide delay fades it out. Outside
	// the RV the bar hides purely on the scroll idle timer.
	if (get_viewport() != nullptr && m_recycler_view != nullptr) {
		const Vector2 mouse = get_viewport()->get_mouse_position();
		if (m_recycler_view->get_global_rect().has_point(mouse)) {
			if (m_mouse_tracked && mouse != m_last_mouse) {
				m_idle_time = 0.0;
			}
			m_last_mouse = mouse;
			m_mouse_tracked = true;
		} else {
			m_mouse_tracked = false;
		}
	}
	// Hover state for the thumb's "grabber_highlight" stylebox (like the
	// built-in ScrollBar, the whole bar counts as hover). Only polled while the
	// bar is interactive (mouse filter STOP); hover only changes the stylebox,
	// never the fade, so it cannot jitter the auto-hide.
	if (get_viewport() != nullptr && get_mouse_filter() != MOUSE_FILTER_IGNORE) {
		const bool hovered = get_global_rect().has_point(get_viewport()->get_mouse_position());
		if (hovered != m_hovered) {
			m_hovered = hovered;
			queue_redraw();
		}
	}
	// Show while dragging, scrolling, or briefly after a scroll; fade out once
	// the RV has sat idle past the delay.
	bool interacting = m_dragging;
	if (m_recycler_view != nullptr
			&& m_recycler_view->get_scroll_state() != RecyclerView::SCROLL_STATE_IDLE) {
		interacting = true;
	}
	if (!get_auto_hide() || interacting || m_idle_time < get_hide_delay()) {
		m_target_alpha = 1.0f;
	} else {
		m_target_alpha = 0.0f;
	}
	m_idle_time += p_delta;
	update_alpha((float)p_delta);
}

void DefaultScrollBar::update_alpha(float p_delta) {
	if (m_alpha < m_target_alpha) {
		m_alpha = MIN(m_alpha + FADE_SPEED * p_delta, m_target_alpha);
	} else if (m_alpha > m_target_alpha) {
		m_alpha = MAX(m_alpha - FADE_SPEED * p_delta, m_target_alpha);
	}
	Color c = get_modulate();
	c.a = m_alpha;
	set_modulate(c);
	const MouseFilter new_filter = m_alpha > 0.01f ? MOUSE_FILTER_STOP : MOUSE_FILTER_IGNORE;
	set_mouse_filter(new_filter);
}

} // namespace godot
