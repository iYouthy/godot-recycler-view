#include "recycler.h"

#include "update_op_apply.h"

#include <godot_cpp/core/error_macros.hpp>

namespace godot {

void Recycler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_adapter", "adapter"), &Recycler::set_adapter);
	ClassDB::bind_method(D_METHOD("get_adapter"), &Recycler::get_adapter);
	ClassDB::bind_method(D_METHOD("set_view_cache_size", "size"), &Recycler::set_view_cache_size);
	ClassDB::bind_method(D_METHOD("get_view_cache_size"), &Recycler::get_view_cache_size);
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
	for (int i = 0; i < m_changed_scrap.size(); i++) {
		Ref<ViewHolder> scrap = m_changed_scrap[i];
		if (scrap->get_position() == p_position && scrap->get_item_view_type() == type) {
			m_changed_scrap.remove_at(i);
			m_adapter->bind_view_holder(scrap, p_position);
			return scrap;
		}
	}

	// 1. View cache: match by layout position and view type.
	for (int i = 0; i < m_cached_views.size(); i++) {
		Ref<ViewHolder> cached = m_cached_views[i];
		if (cached->get_position() == p_position && cached->get_item_view_type() == type) {
			m_cached_views.remove_at(i);
			if (m_adapter->has_stable_ids()) {
				cached->set_stable_id(m_adapter->get_item_id(p_position));
			}
			m_adapter->bind_view_holder(cached, p_position);
			return cached;
		}
	}

	// 2. Recycled pool, by view type.
	void *pooled = m_pool.get_recycled_view(type);
	if (pooled != nullptr) {
		for (int i = 0; i < m_pool_holders.size(); i++) {
			if (m_pool_holders[i].ptr() == pooled) {
				Ref<ViewHolder> holder = m_pool_holders[i];
				m_pool_holders.remove_at(i);
				holder->reset_internal();
				holder->set_item_view_type(type);
				if (m_adapter->has_stable_ids()) {
					holder->set_stable_id(m_adapter->get_item_id(p_position));
				}
				m_adapter->bind_view_holder(holder, p_position);
				return holder;
			}
		}
	}

	// 3. Create a fresh holder.
	Ref<ViewHolder> holder = m_adapter->create_view_holder(nullptr, type);
	if (holder.is_valid()) {
		if (m_adapter->has_stable_ids()) {
			holder->set_stable_id(m_adapter->get_item_id(p_position));
		}
		m_adapter->bind_view_holder(holder, p_position);
	}
	return holder;
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
		victim->reset_internal();
		m_pool_holders.push_back(victim);
		m_pool.put_recycled_view(victim.ptr(), victim->get_item_view_type());
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
