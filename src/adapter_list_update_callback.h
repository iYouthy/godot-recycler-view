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
	void set_adapter(const Ref<Adapter> &p_adapter);
	Ref<Adapter> get_adapter() const;

	void on_inserted(int p_position, int p_count) override;
	void on_removed(int p_position, int p_count) override;
	void on_moved(int p_from_position, int p_to_position) override;
	void on_changed(int p_position, int p_count, const Variant &p_payload) override;

private:
	Ref<Adapter> m_adapter;
};

} // namespace godot
