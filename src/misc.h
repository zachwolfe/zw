#pragma once

#include <concepts>

std::integral auto nearest_multiple_of(std::integral auto x, std::integral auto fac) {
    std::integral auto remainder = x % fac;
    if(remainder == 0) {
        return x;
    } else {
        return x + fac - remainder;
    }
}

std::integral auto nearest_multiple_of_256(std::integral auto x) {
    return nearest_multiple_of(x, 256);
}