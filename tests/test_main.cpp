#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Standalone doctest runner for the pure-algorithm layer.
//
// The algorithm uses godot-cpp header-only containers (godot::Vector<T>),
// which route allocation through godot::Memory and report errors through
// godot::_err_print_*. Since this binary runs without a Godot instance, we
// provide plain-C stubs for those entry points instead of linking godot-cpp's
// compiled core. Our code never triggers the error paths during normal tests.

#include <cstdint>
#include <cstdlib>

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>

namespace godot {

void *Memory::alloc_static(size_t p_bytes, bool p_pad_align) {
	(void)p_pad_align; // Padding is symmetric between alloc and free; irrelevant for tests.
	return std::malloc(p_bytes);
}

void *Memory::realloc_static(void *p_memory, size_t p_bytes, bool p_pad_align) {
	(void)p_pad_align;
	return std::realloc(p_memory, p_bytes);
}

void Memory::free_static(void *p_ptr, bool p_pad_align) {
	(void)p_pad_align;
	std::free(p_ptr);
}

void _err_print_error(const char *, const char *, int, const char *, bool, bool) {}
void _err_print_error(const char *, const char *, int, const String &, bool, bool) {}
void _err_print_error(const char *, const char *, int, const char *, const char *, bool, bool) {}
void _err_print_error(const char *, const char *, int, const String &, const char *, bool, bool) {}
void _err_print_error(const char *, const char *, int, const char *, const String &, bool, bool) {}
void _err_print_error(const char *, const char *, int, const String &, const String &, bool, bool) {}

void _err_print_index_error(const char *, const char *, int, int64_t, int64_t, const char *, const char *, const char *, bool, bool) {}
void _err_print_index_error(const char *, const char *, int, int64_t, int64_t, const char *, const char *, const String &, bool, bool) {}

void _err_flush_stdout() {}

} // namespace godot
