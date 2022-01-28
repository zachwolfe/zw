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

template<typename Char>
struct GenericStringSlice {
    const Char* data = 0;
    size_t size = 0;

    constexpr GenericStringSlice() = default;
    constexpr GenericStringSlice(const Char* data, size_t size) : data(data), size(size) {}
    constexpr GenericStringSlice(const Char* c_string) : data(c_string) {
        size = impl::c_str_size(c_string);
    }

    constexpr Char operator[](size_t i) {
        assert(i < size);
        return data[i];
    }

    constexpr GenericStringSlice operator[](Range range) {
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
    size_t size() const { return characters.size() ? characters.size() - 1 : 0; }
    operator GenericStringSlice<Char>() const {
        return GenericStringSlice<Char>(data(), size());
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

    bool starts_with(GenericStringSlice<Char> slice) {
        if(slice.size > size()) {
            return false;
        }

        for(size_t i = 0; i < slice.size; i++) {
            if(slice[i] != characters.data()[i]) {
                return false;
            }
        }

        return true;
    }
};

};