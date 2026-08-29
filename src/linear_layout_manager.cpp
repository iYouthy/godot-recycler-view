#include "linear_layout_manager.h"

#include "layout_math.h"
#include "recycler_view.h"
#include "state.h"

#include <godot_cpp/core/math.hpp>

namespace godot {

void LinearLayoutManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_orientation", "orientation"), &LinearLayoutManager::set_orientation);
	ClassDB::bind_method(D_METHOD("get_orientation"), &LinearLayoutManager::get_orientation);
	ClassDB::bind_method(D_METHOD("get_item_offset", "position"), &LinearLayoutManager::get_item_offset);
	ClassDB::bind_method(D_METHOD("get_cached_item_count"), &LinearLayoutManager::get_cached_item_count);

	ClassDB::bind_integer_constant(get_class_static(), "Orientation", "VERTICAL", VERTICAL);
	ClassDB::bind_integer_constant(get_class_static(), "Orientation", "HORIZONTAL", HORIZONTAL);
}

void LinearLayoutManager::set_orientation(int p_orientation) {
	m_orientation = p_orientation;
}

void LinearLayoutManager::on_data_changed() {
	m_offsets_dirty = true;
}

int LinearLayoutManager::get_content_size(RecyclerView *p_recycler_view) const {
	const int item_count = get_item_count();
	if (item_count == 0) {
		return 0;
	}
	build_layout(p_recycler_view, item_count);
	return content_size();
}

int LinearLayoutManager::get_item_offset(int p_position) const {
	if (p_position < 0 || p_position >= get_cached_item_count()) {
		return 0;
	}
	return m_offsets[p_position];
}

Rect2 LinearLayoutManager::get_item_rect(RecyclerView *p_recycler_view, int p_position) const {
	if (p_position < 0 || p_position >= get_cached_item_count()) {
		return Rect2();
	}
	const int scroll = m_orientation == VERTICAL
			? p_recycler_view->get_scroll_offset()
			: p_recycler_view->get_scroll_offset_horizontal();
	const Vector2 viewport = p_recycler_view->get_viewport_size();
	const int main_length = p_recycler_view->get_item_height(p_position);
	// reverse_layout flips only the content->screen mapping (item bottom-aligned
	// instead of top-aligned); the scroll offset space is unchanged.
	int offset = m_offsets[p_position] - scroll;
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL ? (int)viewport.y : (int)viewport.x;
		offset = viewport_main - (m_offsets[p_position] + main_length) + scroll;
	}
	if (m_orientation == VERTICAL) {
		return Rect2(0.0f, (float)offset, viewport.x, (float)main_length);
	} else {
		return Rect2((float)offset, 0.0f, (float)main_length, viewport.y);
	}
}

void LinearLayoutManager::build_layout(RecyclerView *p_recycler_view, int p_item_count) const {
	if (!m_offsets_dirty && m_cached_item_count == p_item_count) {
		return;
	}
	m_offsets.resize(p_item_count + 1);
	m_offsets.write[0] = 0;
	for (int i = 0; i < p_item_count; i++) {
		m_offsets.write[i + 1] = m_offsets[i] + p_recycler_view->get_item_height(i);
	}
	m_offsets_dirty = false;
	m_cached_item_count = p_item_count;
}

int LinearLayoutManager::content_size() const {
	return m_offsets[get_cached_item_count()];
}

int LinearLayoutManager::first_visible_position(int p_scroll_offset, int p_item_count) const {
	// First item whose end coordinate exceeds the scroll offset.
	const int upper = upper_bound(m_offsets, p_item_count + 1, p_scroll_offset);
	return CLAMP(upper - 1, 0, p_item_count - 1);
}

int LinearLayoutManager::last_visible_position(int p_scroll_end, int p_item_count) const {
	// First item that starts at or beyond the viewport's bottom edge.
	return MIN(lower_bound(m_offsets, p_item_count + 1, p_scroll_end), p_item_count);
}

void LinearLayoutManager::position_holder(RecyclerView *p_recycler_view, const Ref<ViewHolder> &p_holder, int p_position, int p_scroll_offset) const {
	const Vector2 viewport = p_recycler_view->get_viewport_size();
	const int main_length = p_recycler_view->get_item_height(p_position);
	int offset = m_offsets[p_position] - p_scroll_offset;
	if (is_reverse_layout()) {
		const int viewport_main = m_orientation == VERTICAL ? (int)viewport.y : (int)viewport.x;
		offset = viewport_main - (m_offsets[p_position] + main_length) + p_scroll_offset;
	}
	if (m_orientation == VERTICAL) {
		p_recycler_view->set_item_view_position(p_holder, Vector2(0.0f, (float)offset), Vector2(viewport.x, (float)main_length));
	} else {
		p_recycler_view->set_item_view_position(p_holder, Vector2((float)offset, 0.0f), Vector2((float)main_length, viewport.y));
	}
}

void LinearLayoutManager::on_layout_children(RecyclerView *p_recycler_view, State *p_state) {
	(void)p_state;
	const int item_count = get_item_count();
	if (item_count == 0) {
		return;
	}
	build_layout(p_recycler_view, item_count);

	const int viewport_size = m_orientation == VERTICAL
			? (int)p_recycler_view->get_viewport_size().y
			: (int)p_recycler_view->get_viewport_size().x;
	const int scroll_offset = m_orientation == VERTICAL
			? p_recycler_view->get_scroll_offset()
			: p_recycler_view->get_scroll_offset_horizontal();

	const int first_visible = first_visible_position(scroll_offset, item_count);
	const int last_visible = last_visible_position(scroll_offset + viewport_size, item_count);

	// Recycle holders that scrolled out of the visible range. A holder the
	// touch helper is dragging/swiping/settling stays put (its data position
	// may be out of range even though it is pinned under the finger).
	for (int i = p_recycler_view->get_child_holder_count() - 1; i >= 0; i--) {
		Ref<ViewHolder> holder = p_recycler_view->get_child_holder_at(i);
		const int pos = holder->get_position();
		if (pos < first_visible || pos >= last_visible) {
			if (p_recycler_view->is_item_touch_occupied(holder)) {
				continue;
			}
			// An animated holder scrolled fully out of view: cancel its
			// animation so it can be recycled now. Without this, rapid scrolling
			// past a mutation leaves every animated holder out of view (blocking
			// recycling) and fabricates a fresh view per animation pass.
			const Ref<ItemAnimator> &animator = p_recycler_view->get_item_animator();
			if (animator.is_valid() && animator->is_animating(holder)) {
				animator->cancel_holder(holder);
			}
			p_recycler_view->remove_item_view(holder);
			p_recycler_view->recycle_view(holder, pos);
		}
	}

	// Reposition every remaining holder so the content follows the scroll offset.
	for (int i = 0; i < p_recycler_view->get_child_holder_count(); i++) {
		const Ref<ViewHolder> &holder = p_recycler_view->get_child_holder_at(i);
		position_holder(p_recycler_view, holder, holder->get_position(), scroll_offset);
	}

	// Obtain and position the visible items.
	for (int pos = first_visible; pos < last_visible; pos++) {
		if (has_child_at(p_recycler_view, pos)) {
			continue;
		}
		Ref<ViewHolder> holder = p_recycler_view->get_view_for_position(pos);
		if (holder.is_null()) {
			continue;
		}
		p_recycler_view->add_item_view(holder);
		position_holder(p_recycler_view, holder, pos, scroll_offset);
	}
}

void LinearLayoutManager::collect_adjacent_prefetch_positions(int p_dy, RecyclerView *p_recycler_view, Array &r_positions) const {
	const int item_count = get_item_count();
	if (item_count == 0) {
		return;
	}
	build_layout(p_recycler_view, item_count);
	const int viewport_size = m_orientation == VERTICAL
			? (int)p_recycler_view->get_viewport_size().y
			: (int)p_recycler_view->get_viewport_size().x;
	const int scroll_offset = m_orientation == VERTICAL
			? p_recycler_view->get_scroll_offset()
			: p_recycler_view->get_scroll_offset_horizontal();
	const int first_visible = first_visible_position(scroll_offset, item_count);
	const int last_visible = last_visible_position(scroll_offset + viewport_size, item_count);

	// A fixed runway of positions just past the viewport in the scroll direction.
	static constexpr int PREFETCH_COUNT = 4;
	if (p_dy > 0) {
		const int end = MIN(last_visible + PREFETCH_COUNT, item_count);
		for (int i = last_visible; i < end; i++) {
			r_positions.push_back(i);
		}
	} else if (p_dy < 0) {
		for (int i = first_visible - 1; i >= MAX(first_visible - PREFETCH_COUNT, 0); i--) {
			r_positions.push_back(i);
		}
	}
}

bool LinearLayoutManager::has_child_at(RecyclerView *p_recycler_view, int p_position) const {
	for (int i = 0; i < p_recycler_view->get_child_holder_count(); i++) {
		if (p_recycler_view->get_child_holder_at(i)->get_position() == p_position) {
			return true;
		}
	}
	return false;
}

} // namespace godot
