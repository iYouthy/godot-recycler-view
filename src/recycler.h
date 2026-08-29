#pragma once

#include "adapter.h"
#include "recycled_view_pool.h"
#include "update_op.h"
#include "view_holder.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of RecyclerView.Recycler (simplified core). Obtains ViewHolders for
// positions (from the changed scrap, then the view cache, then the recycled
// pool, then the adapter) and recycles them. The RecycledViewPool holds the
// pool level; the view cache is a small bounded list keyed by layout position;
// the changed scrap holds holders removed by an update within one layout cycle.
class Recycler : public RefCounted {
	GDCLASS(Recycler, RefCounted)

protected:
	static void _bind_methods();

public:
	void set_adapter(const Ref<Adapter> &p_adapter);
	Ref<Adapter> get_adapter() const;

	// A single position slot: after a big jump (scroll-bar drag) the cache-fallback
	// reuses whatever sits here, so the first recycled holder of a subsequent
	// small scroll overflows straight into the pool and keeps the pool stocked for
	// the scrolling item. A larger cache would hold the overflow back and force a
	// fresh view while the pool drains.
	void set_view_cache_size(int p_size) { m_view_cache_max = p_size; }
	int get_view_cache_size() const { return m_view_cache_max; }

	// Per-view-type recycled-pool capacity (default 5, Android's
	// DEFAULT_MAX_SCRAP). Mirrors RecycledViewPool.setMaxRecycledViews.
	void set_view_pool_size(int p_view_type, int p_max) { m_pool.set_max_recycled_views(p_view_type, p_max); }
	int get_view_pool_size(int p_view_type) const { return m_pool.get_max_recycled_views(p_view_type); }

	// The RecyclerView enables this for the layout that follows a scroll offset
	// jump larger than the viewport (e.g. dragging the scroll bar). The view
	// cache then reuses same-type holders even when their stored position no
	// longer matches, instead of leaving them idle while fresh views are built.
	// Small scrolls keep the cache position-exact (see get_view_for_position).
	void set_cache_fallback_enabled(bool p_enabled) { m_cache_fallback = p_enabled; }

	// Drag buffering, Android's GapWorker-driven view-cache expansion: while the
	// scroll bar is dragged the view cache grows to a full viewport and the cache
	// fallback stays on (see RecyclerView), so recycled holders cycle by type and
	// the created count stays bounded even when the drag scrolls far more per
	// frame than the pool can hold. begin_drag_buffer / end_drag_buffer bracket
	// the drag; end sinks any cache overflow back into the pool.
	void begin_drag_buffer(int p_viewport_capacity);
	void end_drag_buffer();
	bool is_drag_buffering() const { return m_drag_buffering; }

	// Obtains a holder for the given layout position, binding it if reused.
	Ref<ViewHolder> get_view_for_position(int p_position);

	// Returns a holder to the cache (if it fits) or the recycled pool.
	void recycle_view(const Ref<ViewHolder> &p_holder, int p_position);

	// Holds a holder removed by an update for the rest of the layout cycle.
	void scrap_view(const Ref<ViewHolder> &p_holder);

	// Pre-creates an unbound holder for the given position into the recycled
	// pool (skipped when the pool is full for the view type). Scrolling into the
	// position later reuses it instead of instantiating a new view. Mirrors the
	// GapWorker prefetch.
	void prefetch_view(int p_position);

	// Releases every scrapped holder to the cache/pool (or frees its Control if
	// the pool is full). Called at the end of a layout cycle.
	void flush_scrap_to_pool();

	// Applies adapter update ops to the positions of cached views so cache
	// lookups stay consistent after data changes.
	void offset_position_records_for_ops(const Vector<UpdateOp> &p_ops);

	// Total number of ViewHolders currently held (cache + pool + scrap).
	int get_recycled_view_count(int p_view_type) const;
	int get_cached_view_count() const { return m_cached_views.size(); }
	int get_changed_scrap_count() const { return m_changed_scrap.size(); }
	int size() const;

	void clear();

	// Frees the Control of every held ViewHolder (test/teardown helper).
	void free_all_views();

private:
	Ref<Adapter> m_adapter;
	int m_view_cache_max = 1;
	int m_view_cache_max_saved = 1;
	bool m_cache_fallback = false;
	bool m_drag_buffering = false;
	Vector<Ref<ViewHolder>> m_cached_views;
	Vector<Ref<ViewHolder>> m_changed_scrap;
	RecycledViewPool m_pool;
	// Keeps the ViewHolders held by the pure pool alive (the pool stores opaque
	// handles and does not participate in reference counting).
	Vector<Ref<ViewHolder>> m_pool_holders;
};

} // namespace godot
