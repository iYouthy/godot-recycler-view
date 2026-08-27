#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "adapter.h"
#include "adapter_helper.h"
#include "adapter_list_update_callback.h"
#include "diff_util.h"
#include "example_class.h"
#include "grid_layout_manager.h"
#include "item_decoration.h"
#include "layout_manager.h"
#include "linear_layout_manager.h"
#include "list_update_callback.h"
#include "recycler.h"
#include "recycler_view.h"
#include "sorted_list_gd.h"
#include "span_size_lookup.h"
#include "state.h"
#include "view_holder.h"

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(DiffUtil);
	GDREGISTER_CLASS(DiffUtilCallback);
	GDREGISTER_CLASS(DiffResult);
	GDREGISTER_CLASS(ListUpdateCallback);
	GDREGISTER_CLASS(BatchingListUpdateCallback);
	GDREGISTER_CLASS(AdapterListUpdateCallback);
	GDREGISTER_CLASS(AdapterHelper);
	GDREGISTER_CLASS(SortedList);
	GDREGISTER_CLASS(SortedListCallback);
	GDREGISTER_CLASS(ViewHolder);
	GDREGISTER_CLASS(Adapter);
	GDREGISTER_CLASS(AdapterDataObserver);
	GDREGISTER_CLASS(State);
	GDREGISTER_CLASS(Recycler);
	GDREGISTER_ABSTRACT_CLASS(LayoutManager);
	GDREGISTER_CLASS(LinearLayoutManager);
	GDREGISTER_CLASS(GridLayoutManager);
	GDREGISTER_CLASS(SpanSizeLookup);
	GDREGISTER_CLASS(ItemDecoration);
	GDREGISTER_CLASS(RecyclerView);
	GDREGISTER_CLASS(ExampleClass);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT godot_recycler_view_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}