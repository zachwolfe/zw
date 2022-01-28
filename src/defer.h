#pragma once

#include "macros.h"

namespace zw::impl {
    template<typename F> struct Deference {
        F function;
        explicit Deference(F function) : function(function) {}
        ~Deference() {
            function();
        }
    };
};
#define zw_defer(statement, ...) ::zw::impl::Deference ZW_CONCAT(__deference__, __LINE__ ## __VA_ARGS__)([&] {{statement;}})
