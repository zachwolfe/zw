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

    RangeIterator begin() const { return {lower_bound}; }
    RangeIterator end() const { return {lower_bound}; }
};

};