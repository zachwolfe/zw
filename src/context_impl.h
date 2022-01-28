#pragma once

// Don't include this file directly. Include context.h instead.

#include "macros.h"
#include "defer.h"

namespace zw::impl {
    extern thread_local ::zw::Context context;
};

#define zw_set_ctx(name, new_value, ...) \
    auto ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__) = ::zw::impl::context.name; \
    ::zw::impl::context.name = new_value; \
    zw_defer(::zw::impl::context.name = ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__), __VA_ARGS__)
#define zw_get_ctx(name) ([&] { return ::zw::impl::context.name; }())
#define use_temp_allocator() zw_set_ctx(allocator, zw_get_ctx(temp_allocator))
#define using_temp_allocator(code) \
    auto ZW_CONCAT(__allocator_old_value__, __LINE__) = zw_get_ctx(allocator); \
    zw_set_ctx(allocator, zw_get_ctx(temp_allocator), first); \
    code; \
    zw_set_ctx(allocator, ZW_CONCAT(__allocator_old_value__, __LINE__), second);