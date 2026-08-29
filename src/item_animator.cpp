#include "item_animator.h"

namespace godot {

void ItemAnimator::_bind_methods() {
	GDVIRTUAL_BIND(_animate_add, "holder", "from", "to");
	GDVIRTUAL_BIND(_animate_remove, "holder", "from", "to");
	GDVIRTUAL_BIND(_animate_move, "holder", "from", "to");
	GDVIRTUAL_BIND(_animate_change, "holder", "from", "to");
	ClassDB::bind_method(D_METHOD("mark_animating", "holder"), &ItemAnimator::mark_animating);
	ClassDB::bind_method(D_METHOD("unmark_animating", "holder"), &ItemAnimator::unmark_animating);
	ClassDB::bind_method(D_METHOD("is_animating", "holder"), &ItemAnimator::is_animating);
	ClassDB::bind_method(D_METHOD("is_running"), &ItemAnimator::is_running);
}

void ItemAnimator::animate_add(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	mark_animating(p_holder);
	GDVIRTUAL_CALL(_animate_add, p_holder, p_from, p_to);
}

void ItemAnimator::animate_remove(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	mark_animating(p_holder);
	GDVIRTUAL_CALL(_animate_remove, p_holder, p_from, p_to);
}

void ItemAnimator::animate_move(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	mark_animating(p_holder);
	GDVIRTUAL_CALL(_animate_move, p_holder, p_from, p_to);
}

void ItemAnimator::animate_change(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to) {
	mark_animating(p_holder);
	GDVIRTUAL_CALL(_animate_change, p_holder, p_from, p_to);
}

void ItemAnimator::cancel_holder(const Ref<ViewHolder> &p_holder) {
	unmark_animating(p_holder);
}

void ItemAnimator::animate_step(double p_delta) {
	// No-op in the base: DefaultItemAnimator drives the timeline here.
}

void ItemAnimator::clear() {
	m_animating.clear();
}

bool ItemAnimator::is_running() const {
	return !m_animating.is_empty();
}

bool ItemAnimator::is_animating(const Ref<ViewHolder> &p_holder) const {
	for (int i = 0; i < m_animating.size(); i++) {
		if (m_animating[i] == p_holder) {
			return true;
		}
	}
	return false;
}

void ItemAnimator::mark_animating(const Ref<ViewHolder> &p_holder) {
	if (!is_animating(p_holder)) {
		m_animating.push_back(p_holder);
	}
}

void ItemAnimator::unmark_animating(const Ref<ViewHolder> &p_holder) {
	for (int i = 0; i < m_animating.size(); i++) {
		if (m_animating[i] == p_holder) {
			m_animating.remove_at(i);
			return;
		}
	}
}

void ItemAnimator::set_recycler_view(RecyclerView *p_rv) {
	m_recycler_view = p_rv;
}

} // namespace godot
