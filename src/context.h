#pragma once
#include <stdint.h>

namespace zw {
    class Allocator;
};
namespace zw {
    class Printer;
};

namespace zw {

// Initialized in context.cpp
struct Context {
    uint32_t indent;
    Allocator* allocator;
    Allocator* temp_allocator;
    Printer* printer;
    bool is_explicitly_copying;
};

};

#include "context_impl.h"