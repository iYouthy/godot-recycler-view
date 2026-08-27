#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.ListUpdateCallback.
// Abstract base class; GDScript subclasses override the _on_* methods.
class ListUpdateCallback : public RefCounted {
	GDCLASS(ListUpdateCallback, RefCounted)

protected:
	static void _bind_methods();

public:
	// Script-overridable virtual methods.
	GDVIRTUAL2(_on_inserted, int, int)
	GDVIRTUAL2(_on_removed, int, int)
	GDVIRTUAL2(_on_moved, int, int)
	GDVIRTUAL3(_on_changed, int, int, Variant)

	virtual void on_inserted(int p_position, int p_count);
	virtual void on_removed(int p_position, int p_count);
	virtual void on_moved(int p_from_position, int p_to_position);
	virtual void on_changed(int p_position, int p_count, const Variant &p_payload);
};

// Port of androidx.recyclerview.widget.BatchingListUpdateCallback.
// Coalesces consecutive same-type updates and forwards them to a wrapped
// ListUpdateCallback on dispatch_last_event().
class BatchingListUpdateCallback : public ListUpdateCallback {
	GDCLASS(BatchingListUpdateCallback, ListUpdateCallback)

protected:
	static void _bind_methods();

public:
	enum Type {
		TYPE_NONE = 0,
		TYPE_ADD = 1,
		TYPE_REMOVE = 2,
		TYPE_CHANGE = 3,
		TYPE_MOVE = 4,
	};

	BatchingListUpdateCallback() = default;
	explicit BatchingListUpdateCallback(const Ref<ListUpdateCallback> &p_wrapped);

	void set_wrapped(const Ref<ListUpdateCallback> &p_wrapped);
	Ref<ListUpdateCallback> get_wrapped() const;

	void on_inserted(int p_position, int p_count) override;
	void on_removed(int p_position, int p_count) override;
	void on_moved(int p_from_position, int p_to_position) override;
	void on_changed(int p_position, int p_count, const Variant &p_payload) override;
	void dispatch_last_event();

	int get_last_event_type() const { return m_last_event_type; }
	int get_last_event_position() const { return m_last_event_position; }
	int get_last_event_count() const { return m_last_event_count; }

private:
	void reset_state();

	Ref<ListUpdateCallback> m_wrapped;
	int m_last_event_type = TYPE_NONE;
	int m_last_event_position = -1;
	int m_last_event_count = -1;
	Variant m_last_event_payload;
};

} // namespace godot
