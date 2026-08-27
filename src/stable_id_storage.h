#pragma once

#include <godot_cpp/templates/vector.hpp>

namespace godot {

// Port of androidx.recyclerview.widget.StableIdStorage. Maps local stable ids
// of nested adapters into a global domain so they do not collide.

constexpr int64_t NO_STABLE_ID = -1;

class StableIdLookup {
public:
	virtual ~StableIdLookup() = default;
	virtual int64_t local_to_global(int64_t p_local_id) = 0;
};

class StableIdStorage {
public:
	virtual ~StableIdStorage() = default;
	virtual StableIdLookup *create_stable_id_lookup() = 0;
};

// Returns NO_STABLE_ID for all positions (stable ids are not supported).
class NoStableIdStorage : public StableIdStorage {
	class NoIdLookup : public StableIdLookup {
	public:
		int64_t local_to_global(int64_t p_local_id) override { return NO_STABLE_ID; }
	};

	NoIdLookup m_lookup;

public:
	StableIdLookup *create_stable_id_lookup() override { return &m_lookup; }
};

// Pass-through: reports the sub-adapter's local stable id as-is.
class SharedPoolStableIdStorage : public StableIdStorage {
	class SameIdLookup : public StableIdLookup {
	public:
		int64_t local_to_global(int64_t p_local_id) override { return p_local_id; }
	};

	SameIdLookup m_lookup;

public:
	StableIdLookup *create_stable_id_lookup() override { return &m_lookup; }
};

// Isolates stable ids: each lookup maps local ids to a globally unique domain.
// All lookups created from the same storage share the id counter.
class IsolatedStableIdStorage : public StableIdStorage {
	int64_t m_next_stable_id = 0;

	class WrapperStableIdLookup : public StableIdLookup {
		IsolatedStableIdStorage *m_storage;
		// Small sparse mapping (local id -> global id); linear search is fine here.
		Vector<int64_t> m_local_ids;
		Vector<int64_t> m_global_ids;

	public:
		explicit WrapperStableIdLookup(IsolatedStableIdStorage *p_storage) :
				m_storage(p_storage) {}

		int64_t local_to_global(int64_t p_local_id) override {
			for (int i = 0; i < m_local_ids.size(); i++) {
				if (m_local_ids[i] == p_local_id) {
					return m_global_ids[i];
				}
			}
			int64_t global_id = m_storage->obtain_id();
			m_local_ids.push_back(p_local_id);
			m_global_ids.push_back(global_id);
			return global_id;
		}
	};

public:
	int64_t obtain_id() { return m_next_stable_id++; }

	StableIdLookup *create_stable_id_lookup() override {
		return new WrapperStableIdLookup(this);
	}
};

} // namespace godot
