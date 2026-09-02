#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {

class RecyclerView;

// Protocol for a RecyclerView scroll bar: a Control child of the RecyclerView.
// The RV binds it (adds it as a child, pins it to the trailing edge), notifies
// it on every scroll/layout via on_scroll_changed, and exposes its data through
// get_offset / get_viewport_size / get_content_size. Subclasses draw themselves
// in _draw and may handle drag input in _gui_input (see DefaultScrollBar).
// GDScript subclasses override _on_scroll_changed to refresh custom bars.
class RecyclerViewScrollBar : public Control {
	GDCLASS(RecyclerViewScrollBar, Control)

protected:
	static void _bind_methods();

	// GDScript hook: called on every scroll/layout. Overriding it takes over
	// entirely — the base class no longer calls queue_redraw(), so the script
	// side must redraw itself.
	GDVIRTUAL0(_on_scroll_changed);

public:
	enum Axis {
		SCROLL_BAR_VERTICAL = 0,
		SCROLL_BAR_HORIZONTAL = 1,
	};

	void set_recycler_view(RecyclerView *p_rv);
	RecyclerView *get_recycler_view() const { return m_recycler_view; }

	void set_axis(int p_axis);
	int get_axis() const { return m_axis; }

	// Cross-axis thickness of the bar (pixels). Default 8.
	void set_thickness(float p_thickness);
	float get_thickness() const { return m_thickness; }

	// Whether the bar fades out while the RV sits idle. DefaultScrollBar honors
	// this; the base stores it so RecyclerView::set_scroll_bar_auto_hide can
	// forward to any RecyclerViewScrollBar implementation.
	virtual void set_auto_hide(bool p_enabled);
	virtual bool get_auto_hide() const { return m_auto_hide; }

	// How long (seconds) the bar stays visible after the last activity (scroll
	// or mouse movement inside the RV) before auto-hiding. Default 0.5s,
	// matching Android's default scrollbar fade delay. Forwarded from
	// RecyclerView::set_scroll_bar_hide_delay.
	virtual void set_hide_delay(float p_delay);
	virtual float get_hide_delay() const { return m_hide_delay; }

	// Called by the RecyclerView after every scroll/layout so the bar can
	// refresh its thumb. Base queues a redraw.
	virtual void on_scroll_changed();

	// Data contract: the bar's axis read from the owning RecyclerView.
	int get_offset() const;
	int get_viewport_size() const;
	int get_content_size() const;

	// Called by RecyclerView::set_scroll_bar: records the RV, picks the axis
	// from the RV's layout and pins the Control to the trailing edge.
	void bind_to(RecyclerView *p_rv);
	void unbind();

protected:
	RecyclerView *m_recycler_view = nullptr;
	int m_axis = SCROLL_BAR_VERTICAL;
	float m_thickness = 8.0f;
	bool m_auto_hide = true;
	float m_hide_delay = 0.5f;
	// Pins the bar to the trailing edge (right for vertical, bottom for
	// horizontal) with the current thickness. Called on bind and when the
	// thickness changes so a wider custom bar re-sizes immediately.
	void apply_anchors();
};

// Default scroll bar: draws a track and a thumb sized by the viewport/content
// ratio, drags the thumb to scroll the RV, and auto-hides after the RV sits
// idle for a while (modulate alpha fade). When the content fits the viewport
// (nothing to scroll) it hides entirely — no track, no thumb, no input —
// regardless of auto_hide. Subclass to tweak appearance or behavior.
class DefaultScrollBar : public RecyclerViewScrollBar {
	GDCLASS(DefaultScrollBar, RecyclerViewScrollBar)

protected:
	static void _bind_methods();

public:
	DefaultScrollBar();

	void _gui_input(const Ref<InputEvent> &p_event) override;
	void _process(double p_delta) override;
	void _draw() override;

	void set_track_color(const Color &p_color);
	Color get_track_color() const { return m_track_color; }
	void set_thumb_color(const Color &p_color);
	Color get_thumb_color() const { return m_thumb_color; }
	void set_corner_radius(float p_radius);
	float get_corner_radius() const { return m_corner_radius; }
	void set_auto_hide(bool p_enabled) override;

	void on_scroll_changed() override;

	// The thumb rectangle in this Control's local space (empty when the content
	// fits the viewport). Exposed for tests.
	Rect2 get_thumb_rect() const;

private:
	// Maps the thumb's start coordinate to a scroll offset and applies it.
	void scroll_to_pos(float p_thumb_start);
	// Progressive drag (Android's handleScrollBarDragging): advances the thumb by
	// the mouse delta from the previous event instead of jumping to the cursor.
	void scroll_by_delta(float p_delta);
	// While the thumb is dragged, grows the Recycler's view cache to a full
	// viewport and keeps the cache fallback on (see Recycler::begin_drag_buffer)
	// so recycled holders cycle by type; restored when the drag ends.
	void begin_drag_buffer();
	void end_drag_buffer();
	// Advances the fade toward the current target alpha.
	void update_alpha(float p_delta);

	static constexpr float MIN_THUMB = 24.0f;
	static constexpr float FADE_SPEED = 6.0f;

	Color m_track_color = Color(0.0f, 0.0f, 0.0f, 0.2f);
	Color m_thumb_color = Color(0.5f, 0.5f, 0.5f, 0.7f);
	float m_corner_radius = 2.0f;
	float m_alpha = 0.0f;
	float m_target_alpha = 0.0f;
	double m_idle_time = 0.0;
	// Last observed scroll offset: the idle timer only resets when the offset
	// actually moves (scrolling), not on every layout (a data mutation re-lays
	// out and re-notifies, but the bar should stay hidden, not flash).
	int m_last_offset = -1;
	// Mouse tracking: when the cursor is inside the RV, moving it resets the
	// idle timer (the bar stays visible), and only cursor stillness past the
	// hide delay fades it out.
	Vector2 m_last_mouse = Vector2(0.0f, 0.0f);
	bool m_mouse_tracked = false;
	// Whether the cursor is over the bar (the thumb draws the
	// "grabber_highlight" theme stylebox, mirroring the built-in ScrollBar's
	// self-managed hover state). Only polled while the bar is interactive.
	bool m_hovered = false;
	bool m_dragging = false;
	// True while the drag's viewport-sized cache buffer is active.
	bool m_drag_buffered = false;
	// Last mouse position along the scroll axis; the drag advances the thumb by
	// the delta from this baseline (Android's incremental scroll-bar dragging).
	float m_last_along = 0.0f;
};

} // namespace godot
