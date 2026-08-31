#include "recycler.h"

#include "update_op_apply.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void Recycler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_adapter", "adapter"), &Recycler::set_adapter);
	ClassDB::bind_method(D_METHOD("get_adapter"), &Recycler::get_adapter);
	ClassDB::bind_method(D_METHOD("set_view_cache_size", "size"), &Recycler::set_view_cache_size);
	ClassDB::bind_method(D_METHOD("get_view_cache_size"), &Recycler::get_view_cache_size);
	ClassDB::bind_method(D_METHOD("update_view_cache_size", "observed"), &Recycler::update_view_cache_size);
	ClassDB::bind_method(D_METHOD("set_view_pool_size", "view_type", "max"), &Recycler::set_view_pool_size);
	ClassDB::bind_method(D_METHOD("get_view_pool_size", "view_type"), &Recycler::get_view_pool_size);
	ClassDB::bind_method(D_METHOD("get_view_for_position", "position"), &Recycler::get_view_for_position);
	ClassDB::bind_method(D_METHOD("recycle_view", "holder", "position"), &Recycler::recycle_view);
	ClassDB::bind_method(D_METHOD("scrap_view", "holder"), &Recycler::scrap_view);
	ClassDB::bind_method(D_METHOD("flush_scrap_to_pool"), &Recycler::flush_scrap_to_pool);
	ClassDB::bind_method(D_METHOD("prefetch_view", "position"), &Recycler::prefetch_view);
	ClassDB::bind_method(D_METHOD("get_recycled_view_count", "view_type"), &Recycler::get_recycled_view_count);
	ClassDB::bind_method(D_METHOD("get_cached_view_count"), &Recycler::get_cached_view_count);
	ClassDB::bind_method(D_METHOD("get_changed_scrap_count"), &Recycler::get_changed_scrap_count);
	ClassDB::bind_method(D_METHOD("size"), &Recycler::size);
	ClassDB::bind_method(D_METHOD("clear"), &Recycler::clear);
}

void Recycler::set_adapter(const Ref<Adapter> &p_adapter) {
	m_adapter = p_adapter;
}

Ref<Adapter> Recycler::get_adapter() const {
	return m_adapter;
}

Ref<ViewHolder> Recycler::get_view_for_position(int p_position) {
	ERR_FAIL_NULL_V(m_adapter, Ref<ViewHolder>());
	const int type = m_adapter->get_item_view_type(p_position);

	// 0. Changed scrap: a holder dropped by an update in this layout cycle.
	// These holders were mounted before (their scene _ready already ran), so a
	// bind here is safe. Its FLAG_BOUND survives (no reset), so the mount in
	// add_item_view skips the re-bind — position matched, content unchanged.
	for (int i = 0; i < m_changed_scrap.size(); i++) {
		Ref<ViewHolder> scrap = m_changed_scrap[i];
		if (scrap->get_position() == p_position && scrap->get_item_view_type() == type) {
			m_changed_scrap.remove_at(i);
			m_adapter->bind_view_holder(scrap, p_position);
			return scrap;
		}
	}

	// 1. View cache: match by layout position and view type.
	// No bind here: the holder is off-tree, and a freshly prefetched one has
	// never entered the tree, so its scene's _ready/@onready references are
	// still null. The bind is deferred to add_item_view, which mounts the
	// control (running the ready pass) before calling the adapter's _bind_item.
	for (int i = 0; i < m_cached_views.size(); i++) {
		Ref<ViewHolder> cached = m_cached_views[i];
		if (cached->get_position() == p_position && cached->get_item_view_type() == type) {
			m_cached_views.remove_at(i);
			cached->set_position(p_position);
			if (m_adapter->has_stable_ids()) {
				cached->set_stable_id(m_adapter->get_item_id(p_position));
			}
			return cached;
		}
	}

	// 1.25 Cache overflow to the pool on a miss: the cache capacity grows with
	// the visible count (see update_view_cache_size), so recycled holders fill
	// the cache and would starve the pool. Each miss here moves the oldest
	// cached holder into the pool instead (port of RecyclerView.Recycler's
	// recycleCachedViewAt, dispatched the same way), keeping the pool fed for
	// the fill loop. Big jumps take the fallback path below instead.
	if (!m_cache_fallback && !m_cached_views.is_empty()) {
		Ref<ViewHolder> victim = m_cached_views[0];
		m_cached_views.remove_at(0);
		if (m_adapter.is_valid()) {
			m_adapter->on_view_recycled(victim);
		}
		victim->reset_internal();
		const int vtype = victim->get_item_view_type();
		if (m_pool.get_recycled_view_count(vtype) >= m_pool.get_max_recycled_views(vtype)) {
			void *oldest = m_pool.get_oldest_recycled_view(vtype);
			for (int i = 0; i < m_pool_holders.size(); i++) {
				if (m_pool_holders[i].ptr() == oldest) {
					Control *control = m_pool_holders[i]->get_control();
					if (control != nullptr) {
						memdelete(control);
					}
					m_pool_holders.remove_at(i);
					break;
				}
			}
		}
		m_pool_holders.push_back(victim);
		m_pool.put_recycled_view(victim.ptr(), vtype);
	}

	// 1.5 Cache fallback (big jumps only): a scroll-bar drag jumps the offset
	// past the viewport each frame, so the exact-position match above misses and
	// the cached holders would otherwise sit idle while fresh views are built.
	// Only the RecyclerView enables this for large jumps; small scrolls keep the
	// cache position-exact so a reverse scroll re-hits the recycled position.
	if (m_cache_fallback) {
		for (int i = 0; i < m_cached_views.size(); i++) {
			Ref<ViewHolder> cached = m_cached_views[i];
			if (cached->get_item_view_type() != type) {
				continue;
			}
			m_cached_views.remove_at(i);
			cached->reset_internal();
			cached->set_item_view_type(type);
			cached->set_position(p_position);
			if (m_adapter->has_stable_ids()) {
				cached->set_stable_id(m_adapter->get_item_id(p_position));
			}
			return cached;
		}
	}

	// 2. Recycled pool, by view type. Deferred bind as in the cache branch:
	// the holder may have been prefetched and never entered the tree.
	void *pooled = m_pool.get_recycled_view(type);
	if (pooled != nullptr) {
		for (int i = 0; i < m_pool_holders.size(); i++) {
			if (m_pool_holders[i].ptr() == pooled) {
				Ref<ViewHolder> holder = m_pool_holders[i];
				m_pool_holders.remove_at(i);
				holder->reset_internal();
				holder->set_item_view_type(type);
				holder->set_position(p_position);
				if (m_adapter->has_stable_ids()) {
					holder->set_stable_id(m_adapter->get_item_id(p_position));
				}
				return holder;
			}
		}
	}

	// 3. Create a fresh holder. Its control never entered the tree, so binding
	// here would run _bind_item against an unready scene (@onready refs null).
	// add_item_view mounts it (the ready pass runs) and binds it there.
	Ref<ViewHolder> holder = m_adapter->create_view_holder(nullptr, type);
	if (holder.is_valid()) {
		holder->set_position(p_position);
		if (m_adapter->has_stable_ids()) {
			holder->set_stable_id(m_adapter->get_item_id(p_position));
		}
	}
	return holder;
}

void Recycler::begin_drag_buffer(int p_viewport_capacity) {
	m_view_cache_max_saved = m_view_cache_max;
	m_view_cache_max = MAX(m_view_cache_max, p_viewport_capacity);
	m_drag_buffering = true;
}

void Recycler::end_drag_buffer() {
	m_drag_buffering = false;
	m_view_cache_max = m_view_cache_max_saved;
	// Sink the cache overflow (grown to a viewport while dragging) back into the
	// pool; recycle_view re-dispatches it (pool-full frees the Control).
	while (m_cached_views.size() > m_view_cache_max) {
		Ref<ViewHolder> holder = m_cached_views[0];
		m_cached_views.remove_at(0);
		recycle_view(holder, holder->get_position());
	}
}

void Recycler::recycle_view(const Ref<ViewHolder> &p_holder, int p_position) {
	ERR_FAIL_NULL(p_holder);
	p_holder->set_position(p_position);

	if (m_cached_views.size() < m_view_cache_max) {
		m_cached_views.push_back(p_holder);
		return;
	}

	// Cache full: move the oldest cached holder to the pool, then cache this one.
	if (!m_cached_views.is_empty()) {
		Ref<ViewHolder> victim = m_cached_views[0];
		m_cached_views.remove_at(0);
		// Port of RecyclerView.Recycler.dispatchViewRecycled (cache overflow:
		// recycleCachedViewAt): the view is about to lose its data and be reused
		// for a different item, so tell the adapter before clearing the holder
		// (its position is still readable here). Views that stay in the cache are
		// kept as-is and never dispatch this.
		if (m_adapter.is_valid()) {
			m_adapter->on_view_recycled(victim);
		}
		victim->reset_internal();
		const int type = victim->get_item_view_type();
		if (m_pool.get_recycled_view_count(type) >= m_pool.get_max_recycled_views(type)) {
			// Pool full for this type: evict the oldest pooled holder so recycled
			// views keep cycling (the freshest are the most likely to be reused by
			// the scrolling item) instead of discarding the incoming one.
			void *oldest = m_pool.get_oldest_recycled_view(type);
			for (int i = 0; i < m_pool_holders.size(); i++) {
				if (m_pool_holders[i].ptr() == oldest) {
					Control *control = m_pool_holders[i]->get_control();
					if (control != nullptr) {
						memdelete(control);
					}
					m_pool_holders.remove_at(i);
					break;
				}
			}
		}
		m_pool_holders.push_back(victim);
		m_pool.put_recycled_view(victim.ptr(), type);
	}
	m_cached_views.push_back(p_holder);
}

void Recycler::scrap_view(const Ref<ViewHolder> &p_holder) {
	ERR_FAIL_NULL(p_holder);
	p_holder->add_flags(ViewHolder::FLAG_RETURNED_FROM_SCRAP);
	m_changed_scrap.push_back(p_holder);
}

void Recycler::flush_scrap_to_pool() {
	for (int i = 0; i < m_changed_scrap.size(); i++) {
		Ref<ViewHolder> holder = m_changed_scrap[i];
		// Port of Recycler.dispatchViewRecycled (scrap -> pool): the holder was
		// dropped by an update and is not reused in this layout cycle, so it is
		// about to lose its data. Dispatched even when the pool is full and the
		// view is discarded below (Android dispatches before putRecycledView).
		if (m_adapter.is_valid()) {
			m_adapter->on_view_recycled(holder);
		}
		holder->reset_internal();
		const int type = holder->get_item_view_type();
		if (m_pool.get_recycled_view_count(type) < m_pool.get_max_recycled_views(type)) {
			m_pool_holders.push_back(holder);
			m_pool.put_recycled_view(holder.ptr(), type);
		} else {
			Control *control = holder->get_control();
			if (control != nullptr) {
				memdelete(control);
			}
		}
	}
	m_changed_scrap.clear();
}

void Recycler::prefetch_view(int p_position) {
	ERR_FAIL_NULL(m_adapter);
	if (p_position < 0 || p_position >= m_adapter->get_item_count()) {
		return;
	}
	const int type = m_adapter->get_item_view_type(p_position);
	if (m_pool.get_recycled_view_count(type) >= m_pool.get_max_recycled_views(type)) {
		return;  // Pool full for this view type; prefetching more would evict.
	}
	// A pooled holder of this type already covers the scrolling item; creating
	// another here would fabricate a fresh view on every layout pass.
	if (m_pool.get_recycled_view_count(type) > 0) {
		return;
	}
	// Cached holders of this type flow back into the pool as the cache fills, so
	// the scrolling items will reuse them; nothing to prefetch yet.
	for (int i = 0; i < m_cached_views.size(); i++) {
		if (m_cached_views[i]->get_item_view_type() == type) {
			return;
		}
	}
	Ref<ViewHolder> holder = m_adapter->create_view_holder(nullptr, type);
	if (holder.is_valid()) {
		holder->set_item_view_type(type);
		m_pool_holders.push_back(holder);
		m_pool.put_recycled_view(holder.ptr(), type);
	}
}

void Recycler::offset_position_records_for_ops(const Vector<UpdateOp> &p_ops) {
	for (int i = 0; i < m_cached_views.size(); i++) {
		Ref<ViewHolder> holder = m_cached_views[i];
		int position = holder->get_position();
		for (int j = 0; j < p_ops.size(); j++) {
			const HolderUpdateEffect effect = apply_update_op_to_holder(p_ops[j], position);
			if (effect.removed) {
				position = NO_POSITION;
				break;
			}
			position = effect.position;
		}
		holder->set_position(position);
	}
}

int Recycler::get_recycled_view_count(int p_view_type) const {
	return m_pool.get_recycled_view_count(p_view_type);
}

int Recycler::size() const {
	return m_cached_views.size() + m_changed_scrap.size() + m_pool_holders.size();
}

void Recycler::clear() {
	m_cached_views.clear();
	m_changed_scrap.clear();
	m_pool_holders.clear();
	m_pool.clear();
}

void Recycler::free_all_views() {
	for (int i = 0; i < m_cached_views.size(); i++) {
		Control *control = m_cached_views[i]->get_control();
		if (control != nullptr) {
			memdelete(control);
		}
	}
	for (int i = 0; i < m_changed_scrap.size(); i++) {
		Control *control = m_changed_scrap[i]->get_control();
		if (control != nullptr) {
			memdelete(control);
		}
	}
	for (int i = 0; i < m_pool_holders.size(); i++) {
		Control *control = m_pool_holders[i]->get_control();
		if (control != nullptr) {
			memdelete(control);
		}
	}
	clear();
}

} // namespace godot
