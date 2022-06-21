#pragma once

namespace zw {

struct Range {
    size_t lower_bound = 0;
    size_t upper_bound = 0;

    size_t begin() const { return lower_bound; }
    size_t end() const { return upper_bound; }
};

};