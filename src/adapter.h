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
	GDVIRTUAL0R(int, _get_item_count)
	GDVIRTUAL1R(int, _get_item_view_type, int)
	GDVIRTUAL1R(int, _get_item_height, int)
	GDVIRTUAL1R(int64_t, _get_item_id, int)
	GDVIRTUAL1(_on_item_recycled, Ref<ViewHolder>)
	GDVIRTUAL1R(bool, _on_failed_to_recycle_view, Ref<ViewHolder>)
	GDVIRTUAL1(_on_view_attached_to_window, Ref<ViewHolder>)
	GDVIRTUAL1(_on_view_detached_from_window, Ref<ViewHolder>)

	Ref<ViewHolder> create_view_holder(Control *p_parent, int p_view_type);
	void bind_view_holder(const Ref<ViewHolder> &p_holder, int p_position);

	int get_item_count();
	int get_item_view_type(int p_position);
	// Returns the item's height along the scroll axis, or <= 0 for the
	// RecyclerView's default item size (variable heights are optional).
	int get_item_height(int p_position);
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
