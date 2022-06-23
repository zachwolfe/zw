#pragma once

namespace zw {

struct RangeIterator {
    size_t value;

    RangeIterator& operator++() {
        ++value;
        return *this;
    }

    bool operator==(RangeIterator other) const { return value == other.value; }
    bool operator!=(RangeIterator other) const { return value != other.value; }
    size_t operator*() {
        return value;
    }
};

struct Range {
    size_t lower_bound = 0;
    size_t upper_bound = 0;

    Range(size_t lower_bound, size_t upper_bound) : lower_bound(lower_bound), upper_bound(upper_bound) {}
    Range(size_t upper_bound) : lower_bound(0), upper_bound(upper_bound) {}

    RangeIterator begin() const { return {lower_bound}; }
    RangeIterator end() const { return {upper_bound}; }
};

};