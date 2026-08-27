#pragma once

#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/rect2.hpp>

namespace godot {

class RecyclerView;

// Port of RecyclerView.ItemAnimator: animates items after incremental updates.
// The RecyclerView runs a two-phase layout (capturing pre-update positions),
// then dispatches animate_* per holder. DefaultItemAnimator (C++) drives the
// standard add/remove/move/change animations; GDScript subclasses override the
// _animate_* virtuals and manage their own tweening.
class ItemAnimator : public RefCounted {
	GDCLASS(ItemAnimator, RefCounted)

protected:
	static void _bind_methods();

public:
	virtual void animate_add(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to);
	virtual void animate_remove(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to);
	virtual void animate_move(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to);
	virtual void animate_change(const Ref<ViewHolder> &p_holder, const Rect2 &p_from, const Rect2 &p_to);
	// Advances all running animations by p_delta seconds (driven by _process).
	virtual void animate_step(double p_delta);
	// Drops every queued/running animation and releases the held holders.
	virtual void clear();

	bool is_running() const;
	bool is_animating(const Ref<ViewHolder> &p_holder) const;
	void mark_animating(const Ref<ViewHolder> &p_holder);
	void unmark_animating(const Ref<ViewHolder> &p_holder);

	// The owning RecyclerView, used by the DefaultItemAnimator to re-query the
	// layout target and to recycle removed holders on completion.
	void set_recycler_view(RecyclerView *p_rv);

	GDVIRTUAL3(_animate_add, Ref<ViewHolder>, Rect2, Rect2)
	GDVIRTUAL3(_animate_remove, Ref<ViewHolder>, Rect2, Rect2)
	GDVIRTUAL3(_animate_move, Ref<ViewHolder>, Rect2, Rect2)
	GDVIRTUAL3(_animate_change, Ref<ViewHolder>, Rect2, Rect2)

	Vector<Ref<ViewHolder>> m_animating;
	RecyclerView *m_recycler_view = nullptr;
};

} // namespace godot
