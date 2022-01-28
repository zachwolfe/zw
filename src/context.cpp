#include "context.h"
#include "alloc.h"
#include "fmt.h"

namespace zw::impl {
    extern thread_local Context context {
        .indent = 0,
        .allocator = &global_allocator,
        .temp_allocator = &temp_allocator,
        .printer = &stdout_printer,
        .is_explicitly_copying = false,
    };
}
