#include "diff_util.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

namespace {

// Bridges the pure-algorithm DiffListUpdateCallback interface to a Godot
// ListUpdateCallback. Payloads arrive as opaque pointers that point at a
// transient Variant stored on the DiffUtilCallback.
class GodotListUpdateCallbackAdapter : public DiffListUpdateCallback {
	ListUpdateCallback *target = nullptr;

public:
	explicit GodotListUpdateCallbackAdapter(ListUpdateCallback *p_target) :
			target(p_target) {}

	void on_inserted(int p_position, int p_count) override {
		target->on_inserted(p_position, p_count);
	}

	void on_removed(int p_position, int p_count) override {
		target->on_removed(p_position, p_count);
	}

	void on_moved(int p_from_position, int p_to_position) override {
		target->on_moved(p_from_position, p_to_position);
	}

	void on_changed(int p_position, int p_count, const void *p_payload) override {
		const Variant *payload = static_cast<const Variant *>(p_payload);
		target->on_changed(p_position, p_count, payload ? *payload : Variant());
	}
};

} // namespace

// ---------------------------------------------------------------------------
// DiffUtil.

void DiffUtil::_bind_methods() {
	ClassDB::bind_static_method("DiffUtil", D_METHOD("calculate_diff", "callback", "detect_moves"), &DiffUtil::calculate_diff, DEFVAL(true));
}

Ref<DiffResult> DiffUtil::calculate_diff(const Ref<DiffUtilCallback> &p_callback, bool p_detect_moves) {
	ERR_FAIL_NULL_V(p_callback, Ref<DiffResult>());
	Ref<DiffResult> result;
	result.instantiate();
	result->m_callback = p_callback;
	result->m_result_data = memnew(DiffResultData(DiffAlgorithm::calculate_diff(*p_callback.ptr(), p_detect_moves)));
	return result;
}

// ---------------------------------------------------------------------------
// DiffUtilCallback.

void DiffUtilCallback::_bind_methods() {
	GDVIRTUAL_BIND(_get_old_list_size);
	GDVIRTUAL_BIND(_get_new_list_size);
	GDVIRTUAL_BIND(_are_items_the_same, "old_item_position", "new_item_position");
	GDVIRTUAL_BIND(_are_contents_the_same, "old_item_position", "new_item_position");
	GDVIRTUAL_BIND(_get_change_payload, "old_item_position", "new_item_position");
}

int DiffUtilCallback::get_old_list_size() {
	int result = 0;
	GDVIRTUAL_CALL(_get_old_list_size, result);
	return result;
}

int DiffUtilCallback::get_new_list_size() {
	int result = 0;
	GDVIRTUAL_CALL(_get_new_list_size, result);
	return result;
}

bool DiffUtilCallback::are_items_the_same(int p_old_item_position, int p_new_item_position) {
	bool result = false;
	GDVIRTUAL_CALL(_are_items_the_same, p_old_item_position, p_new_item_position, result);
	return result;
}

bool DiffUtilCallback::are_contents_the_same(int p_old_item_position, int p_new_item_position) {
	bool result = false;
	GDVIRTUAL_CALL(_are_contents_the_same, p_old_item_position, p_new_item_position, result);
	return result;
}

const void *DiffUtilCallback::get_change_payload(int p_old_item_position, int p_new_item_position) {
	Variant result;
	if (GDVIRTUAL_CALL(_get_change_payload, p_old_item_position, p_new_item_position, result)) {
		m_payload_buffer = result;
		return &m_payload_buffer;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// DiffResult.

void DiffResult::_bind_methods() {
	ClassDB::bind_method(D_METHOD("convert_old_position_to_new", "old_list_position"), &DiffResult::convert_old_position_to_new);
	ClassDB::bind_method(D_METHOD("convert_new_position_to_old", "new_list_position"), &DiffResult::convert_new_position_to_old);
	ClassDB::bind_method(D_METHOD("dispatch_updates_to", "update_callback"), &DiffResult::dispatch_updates_to);
}

DiffResult::DiffResult() {}

DiffResult::~DiffResult() {
	if (m_result_data != nullptr) {
		memdelete(m_result_data);
	}
}

int DiffResult::convert_old_position_to_new(int p_old_list_position) const {
	ERR_FAIL_NULL_V(m_result_data, DiffResultData::NO_POSITION);
	ERR_FAIL_COND_V_MSG(p_old_list_position < 0 || p_old_list_position >= m_result_data->get_old_list_size(), DiffResultData::NO_POSITION,
			"Index out of bounds - passed position = " + String::num_int64(p_old_list_position) + ", old list size = " + String::num_int64(m_result_data->get_old_list_size()));
	return m_result_data->convert_old_position_to_new(p_old_list_position);
}

int DiffResult::convert_new_position_to_old(int p_new_list_position) const {
	ERR_FAIL_NULL_V(m_result_data, DiffResultData::NO_POSITION);
	ERR_FAIL_COND_V_MSG(p_new_list_position < 0 || p_new_list_position >= m_result_data->get_new_list_size(), DiffResultData::NO_POSITION,
			"Index out of bounds - passed position = " + String::num_int64(p_new_list_position) + ", new list size = " + String::num_int64(m_result_data->get_new_list_size()));
	return m_result_data->convert_new_position_to_old(p_new_list_position);
}

void DiffResult::dispatch_updates_to(const Ref<ListUpdateCallback> &p_update_callback) const {
	ERR_FAIL_NULL(m_result_data);
	ERR_FAIL_NULL(p_update_callback);
	Ref<BatchingListUpdateCallback> batching = p_update_callback;
	if (batching.is_null()) {
		batching.instantiate();
		batching->set_wrapped(p_update_callback);
	}
	GodotListUpdateCallbackAdapter adapter(batching.ptr());
	m_result_data->dispatch_updates_to(adapter);
	batching->dispatch_last_event();
}

} // namespace godot
