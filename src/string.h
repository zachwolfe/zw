#pragma once

#include <assert.h>
#include <windows.h>
#include <stringapiset.h>

#include "array.h"
#include "range.h"
#include "context.h"

namespace zw {

template<typename Char>
class GenericString;

using String = GenericString<char>;
using WideString = GenericString<wchar_t>;

namespace impl {
// Like strlen, but generic and slower.
template<typename Char>

    constexpr inline size_t c_str_size(const Char* c_string) {
        size_t size = 0;
        if(c_string) {
            while(c_string[size] != 0) {
                size++;
            }
        }
        return size;
    }
};

namespace priv {
    inline char to_lower(char c) {
        return (char)tolower(c);
    }

    inline wchar_t to_lower(wchar_t c) {
        return (wchar_t)towlower(c);
    }
};

template<typename Char>
struct GenericStringSlice {
    const Char* data = 0;
    size_t size = 0;

    constexpr GenericStringSlice() = default;
    constexpr GenericStringSlice(const Char* data, size_t size) : data(data), size(size) {}
    constexpr GenericStringSlice(const Char* c_string) : data(c_string) {
        size = impl::c_str_size(c_string);
    }

    constexpr Char operator[](size_t i) const {
        assert(i < size);
        return data[i];
    }

    constexpr GenericStringSlice operator[](Range range) const {
        assert(range.upper_bound <= size);
        assert(range.lower_bound <= range.upper_bound);
        return {&data[range.lower_bound], range.upper_bound - range.lower_bound};
    }

    NoDestruct<GenericString<Char>> c_string() const {
        use_temp_allocator();
        GenericString<Char> val = *this;
        return std::move(val);
    }

    // TODO: is this syntax really necessary?
    template<typename Char=Char>
    requires std::same_as<Char, char>
    NoDestruct<WideString> to_wide_string() const {
        use_temp_allocator();
        WideString val = *this;
        return std::move(val);
    }

    template<typename Char=Char>
    requires std::same_as<Char, wchar_t>
    NoDestruct<String> to_narrow_string() const {
        use_temp_allocator();
        String val = *this;
        return std::move(val);
    }

    bool starts_with(GenericStringSlice<Char> other) const {
        if(other.size > size) return false;

        return memcmp(data, other.data, sizeof(Char) * other.size) == 0;
    }
    bool starts_with_ignoring_case(GenericStringSlice<Char> other) const {
        if(other.size > size) return false;

        for(size_t i = 0; i < other.size; i++) {
            if(priv::to_lower(other[i]) != priv::to_lower((*this)[i])) {
                return false;
            }
        }

        return true;
    }
    bool is_equal_to_ignoring_case(GenericStringSlice<Char> other) const {
        return (size == other.size) && starts_with_ignoring_case(other);
    }
    bool operator==(GenericStringSlice<Char> other) const {
        if(size != other.size) return false;

        return memcmp(data, other.data, sizeof(Char) * size) == 0;
    }
};

using StringSlice = GenericStringSlice<char>;
using WideStringSlice = GenericStringSlice<wchar_t>;

template<typename Char>
class GenericString {
    Array<Char> characters;
public:
    GenericString() = default;
    GenericString(GenericStringSlice<Char> slice) {
        if(slice.size) {
            assert(slice.data);
            characters.reserve(slice.size + 1);
            memcpy(characters.data(), slice.data, slice.size * sizeof(Char));
            characters.unsafe_set_size(slice.size + 1);
            characters[slice.size] = 0;
        }
    }

    GenericString(const Char* c_string) : GenericString(GenericStringSlice<Char>(c_string)) {}

    template<typename Char=Char>
    requires std::same_as<Char, char>
    GenericString(WideStringSlice wide_string) {
        int length = WideCharToMultiByte(CP_UTF8, 0, wide_string.data, (int)wide_string.size, nullptr, 0, nullptr, nullptr);
        characters.reserve(length + 1);
        int code = WideCharToMultiByte(CP_UTF8, 0, wide_string.data, -1, characters.data(), length, nullptr, nullptr);
        characters.unsafe_set_size(length);
        characters[size()] = 0;
    }
    template<typename Char=Char>
    requires std::same_as<Char, char>
    GenericString(const wchar_t* wide_string) : GenericString(WideStringSlice(wide_string)) {}
    

    template<typename Char=Char>
    requires std::same_as<Char, wchar_t>
    GenericString(const char* narrow_string) {
        fflush(stdout);
        int length = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, narrow_string, -1, nullptr, 0);
        characters.reserve(length);
        int code = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, narrow_string, -1, characters.data(), length);
        characters.unsafe_set_size(length);
    }

    template<typename Char=Char>
    requires std::same_as<Char, wchar_t>
    GenericString(StringSlice narrow_string) {
        // TODO: efficiency. It would be nice if StringSlice had a way of knowing if it were null-terminated or not...
        auto null_terminated = narrow_string.c_string();
        *this = null_terminated->data();
    }

    const Char* data() const { return characters.data(); }
    Char* data() { return characters.data(); }
    size_t size() const { return characters.size() ? characters.size() - 1 : 0; }
    operator GenericStringSlice<Char>() const {
        return GenericStringSlice<Char>(data(), size());
    }
    GenericStringSlice<Char> as_slice() const {
        return *this;
    }

    void append(GenericStringSlice<Char> slice) {
        if(slice.size) {
            assert(slice.data);
            size_t og_size = size();
            size_t new_size = og_size + slice.size;
            characters.reserve(new_size + 1);
            memcpy(characters.data() + og_size, slice.data, slice.size * sizeof(Char));
            characters.unsafe_set_size(new_size + 1);
            characters[new_size] = 0;
        }
    }
    void push(Char c) {
        characters[size()] = c;
        characters.push(0);
    }
    void resize(size_t new_size) {
        characters.resize(new_size + 1);
        characters[size()] = 0;
    }

    bool starts_with(GenericStringSlice<Char> slice) const {
        return as_slice().starts_with(slice);
    }
    bool starts_with_ignoring_case(GenericStringSlice<Char> slice) const {
        return as_slice().starts_with_ignoring_case(slice);
    }
    bool is_equal_to_ignoring_case(GenericStringSlice<Char> slice) const {
        return as_slice().is_equal_to_ignoring_case(slice);
    }
    bool operator==(GenericStringSlice<Char> slice) const {
        return as_slice() == slice;
    }
};

};