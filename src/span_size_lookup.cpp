#include "span_size_lookup.h"

namespace godot {

void SpanSizeLookup::_bind_methods() {
	GDVIRTUAL_BIND(_get_span_size, "position");
}

int SpanSizeLookup::get_span_size(int p_position) {
	int result = 1;
	GDVIRTUAL_CALL(_get_span_size, p_position, result);
	return result > 0 ? result : 1;
}

} // namespace godot
