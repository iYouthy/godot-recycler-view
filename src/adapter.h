#pragma once

#include "view_holder.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of RecyclerView.AdapterDataObserver. Receives data-change notifications
// from an Adapter. GDScript subclasses override the _on_* methods.
class AdapterDataObserver : public RefCounted {
	GDCLASS(AdapterDataObserver, RefCounted)

protected:
	static void _bind_methods();

public:
	GDVIRTUAL0(_on_changed)
	GDVIRTUAL3(_on_item_range_changed, int, int, Variant)
	GDVIRTUAL2(_on_item_range_inserted, int, int)
	GDVIRTUAL2(_on_item_range_removed, int, int)
	GDVIRTUAL2(_on_item_moved, int, int)
	GDVIRTUAL0(_on_state_restoration_policy_changed)

	virtual void on_changed();
	virtual void on_item_range_changed(int p_position, int p_count, const Variant &p_payload);
	virtual void on_item_range_inserted(int p_position, int p_count);
	virtual void on_item_range_removed(int p_position, int p_count);
	virtual void on_item_moved(int p_from_position, int p_to_position);
	virtual void on_state_restoration_policy_changed();
};

// Port of RecyclerView.Adapter. Abstract base class; GDScript subclasses
// override the _create_item / _bind_item / _get_item_count methods.
class Adapter : public RefCounted {
	GDCLASS(Adapter, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script-overridable virtual methods.
	GDVIRTUAL2R(Ref<ViewHolder>, _create_item, Control *, int)
	GDVIRTUAL2(_bind_item, Ref<ViewHolder>, int)
	// Optional partial rebind: a change op that carried a payload calls this with
	// the payload, so the adapter can update only the affected child control
	// instead of rebinding the whole item. Falls back to _bind_item when absent.
	GDVIRTUAL3(_bind_item_with_payload, Ref<ViewHolder>, int, Variant)
	GDVIRTUAL0R(int, _get_item_count)
	GDVIRTUAL1R(int, _get_item_view_type, int)
	GDVIRTUAL1R(int, _get_item_extent, int)
	GDVIRTUAL1R(int64_t, _get_item_id, int)
	// Renamed from Android's Adapter.onViewRecycled: the view is about to lose
	// its data (returned to the recycled pool), so release expensive resources.
	GDVIRTUAL1(_on_item_recycled, Ref<ViewHolder>)
	// Renamed from Android's Adapter.onFailedToRecycleView: a holder declared
	// non-recyclable (set_is_recyclable(false)) is about to be recycled; return
	// true to force the recycle, false (default) to keep it attached and
	// re-visit the decision on later layout passes.
	GDVIRTUAL1R(bool, _on_failed_to_recycle_view, Ref<ViewHolder>)
	// Renamed from Android's Adapter.onViewAttachedToWindow (there is no
	// window concept in Godot): the item Control was added to the RecyclerView,
	// i.e. it is about to be seen by the user.
	GDVIRTUAL1(_on_view_attached, Ref<ViewHolder>)
	// Renamed from Android's Adapter.onViewDetachedFromWindow: the item Control
	// was removed from the RecyclerView (scrolled off, removed, or its remove
	// animation finished). Not necessarily permanent: it may be attached again.
	GDVIRTUAL1(_on_view_detached, Ref<ViewHolder>)

	Ref<ViewHolder> create_view_holder(Control *p_parent, int p_view_type);
	void bind_view_holder(const Ref<ViewHolder> &p_holder, int p_position);
	// Binds with a change payload: calls _bind_item_with_payload when the payload
	// is set and the script implements it, otherwise falls back to a full rebind.
	void bind_view_holder_with_payload(const Ref<ViewHolder> &p_holder, int p_position, const Variant &p_payload);

	// Lifecycle dispatchers (called by the RecyclerView / Recycler; see the
	// GDVIRTUAL hooks above for the Android mapping).
	void on_view_recycled(const Ref<ViewHolder> &p_holder);
	bool on_failed_to_recycle_view(const Ref<ViewHolder> &p_holder);
	void on_view_attached(const Ref<ViewHolder> &p_holder);
	void on_view_detached(const Ref<ViewHolder> &p_holder);

	// Virtual so adapter subclasses (e.g. ListAdapter) can derive the count from
	// their own state instead of a script _get_item_count override.
	virtual int get_item_count();
	int get_item_view_type(int p_position);
	// Returns the item's extent along the scroll axis, or <= 0 for the
	// RecyclerView's default item extent (variable extents are optional).
	int get_item_extent(int p_position);
	int64_t get_item_id(int p_position);
	bool has_stable_ids() const { return m_has_stable_ids; }
	void set_has_stable_ids(bool p_has_stable_ids);

	bool has_observers() const { return !m_observers.is_empty(); }
	void register_adapter_data_observer(const Ref<AdapterDataObserver> &p_observer);
	void unregister_adapter_data_observer(const Ref<AdapterDataObserver> &p_observer);

	void notify_data_set_changed();
	void notify_item_range_changed(int p_position, int p_count, const Variant &p_payload);
	void notify_item_range_inserted(int p_position, int p_count);
	void notify_item_range_removed(int p_position, int p_count);
	void notify_item_moved(int p_from_position, int p_to_position);
	void notify_item_changed(int p_position);
	void notify_item_inserted(int p_position);
	void notify_item_removed(int p_position);

private:
	Vector<Ref<AdapterDataObserver>> m_observers;
	bool m_has_stable_ids = false;
};

} // namespace godot
