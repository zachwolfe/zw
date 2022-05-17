#pragma once
#include <stdint.h>

#include "macros.h"
#include "defer.h"

#define ZW_DECLARE_CTX_VAR(type, name) extern thread_local type name##_zw_ctx_var;
#define ZW_DEFINE_CTX_VAR(type, name, expression) thread_local type name##_zw_ctx_var = (expression);

#define zw_set_ctx(name, new_value, ...) \
    auto ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__) = name##_zw_ctx_var; \
    name##_zw_ctx_var = new_value; \
    zw_defer(name##_zw_ctx_var = ZW_CONCAT(__ctx_old_value__, __LINE__ ## __VA_ARGS__), __VA_ARGS__)
#define zw_get_ctx(name) ([&] { return name##_zw_ctx_var; }())
