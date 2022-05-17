#pragma once
#include <stdint.h>

#include "macros.h"
#include "defer.h"

#define ZW_DECLARE_CTX_VAR(type, name) namespace zw::impl::ctx { \
    extern thread_local type name; \
};
#define ZW_DEFINE_CTX_VAR(type, name, expression) \
    type ZW_CONCAT(initializer_for_ ## name ## _on_line, __LINE__) = (expression);\
    \
    namespace zw::impl::ctx { \
        thread_local type name = ZW_CONCAT(initializer_for_ ## name ## _on_line, __LINE__); \
    };

#define zw_set_ctx(name, new_value, ...) \
    auto ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__) = ::zw::impl::ctx::name; \
    ::zw::impl::ctx::name = new_value; \
    zw_defer(::zw::impl::ctx::name = ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__), __VA_ARGS__)
#define zw_get_ctx(name) ([&] { return ::zw::impl::ctx::name; }())
#define use_temp_allocator() zw_set_ctx(allocator, zw_get_ctx(temp_allocator))
#define using_temp_allocator(code) \
    auto ZW_CONCAT(__allocator_old_value__, __LINE__) = zw_get_ctx(allocator); \
    zw_set_ctx(allocator, zw_get_ctx(temp_allocator), first); \
    code; \
    zw_set_ctx(allocator, ZW_CONCAT(__allocator_old_value__, __LINE__), second);
