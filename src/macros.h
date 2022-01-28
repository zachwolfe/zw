#pragma once

#define ZW_CONCAT_IMPL(a,b) a##b
#define ZW_CONCAT(a,b) ZW_CONCAT_IMPL(a,b)

#define ZW_STRINGIFY_IMPL(x) #x
#define ZW_STRINGIFY(x) ZW_STRINGIFY_IMPL(x)