#pragma once

#include "adapter.h"
#include "list_update_callback.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Port of androidx AdapterListUpdateCallback. Bridges a DiffUtil dispatch to the
// Adapter's notify_* methods so a computed diff drives incremental updates.
class AdapterListUpdateCallback : public ListUpdateCallback {
	GDCLASS(AdapterListUpdateCallback, ListUpdateCallback)

protected:
	static void _bind_methods();

public:
	// Raw pointer, not Ref: the adapter owns this callback (ListAdapter holds it
	// in its m_update_callback Ref), so a Ref back to the adapter would form a
	// reference cycle (adapter <-> callback) and leak both. Taking a raw pointer
	// also avoids constructing a temporary Ref<Adapter> from `this` inside the
	// ListAdapter constructor: Ref(T*) uses init_ref(), which releases the
	// engine-side reference on destruction and would free the still-constructed
	// object mid-construction.
	void set_adapter(Adapter *p_adapter);
	Ref<Adapter> get_adapter() const;

	void on_inserted(int p_position, int p_count) override;
	void on_removed(int p_position, int p_count) override;
	void on_moved(int p_from_position, int p_to_position) override;
	void on_changed(int p_position, int p_count, const Variant &p_payload) override;

private:
	// Raw pointer, not Ref: the adapter owns this callback (ListAdapter holds it
	// in its m_update_callback Ref), so a Ref back to the adapter would form a
	// reference cycle (adapter <-> callback) and leak both. The callback is
	// destroyed together with the adapter, so the pointer never dangles while
	// the callback is alive.
	Adapter *m_adapter = nullptr;
};

} // namespace godot
