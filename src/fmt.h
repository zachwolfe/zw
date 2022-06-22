#pragma once

#include <stdio.h>

#include "string.h"
#include "context.h"

namespace zw { class Printer; };

ZW_DECLARE_CTX_VAR(zw::Printer*, printer);
ZW_DECLARE_CTX_VAR(uint32_t, indent);

// Narrow strings
template<typename... Ds>
void zw_print(zw::StringSlice format, Ds&&... ds);

template<typename... Ds>
void zw_println(zw::StringSlice format, Ds&&... ds) {
    zw_print(format, std::forward<Ds>(ds)...);
    zw_print("\n");
}
template<typename... Ds>
zw::String zw_format(zw::StringSlice format, Ds&&... ds);


// Wide strings
template<typename... Ds>
void zw_print(zw::WideStringSlice format, Ds&&... ds);

template<typename... Ds>
void zw_println(zw::WideStringSlice format, Ds&&... ds) {
    zw_print(format, std::forward<Ds>(ds)...);
    zw_print("\n");
}
template<typename... Ds>
zw::WideString zw_format(zw::WideStringSlice format, Ds&&... ds);

namespace zw {

template<typename T>
concept Display = requires(const T& val) {
    { ::zw_display(val) };
};

class Printer {
public:
    virtual void print(StringSlice string) = 0;
    virtual void print(WideStringSlice string) = 0;
};

class StdFilePrinter: public Printer {
    FILE* file;
public:
    StdFilePrinter(FILE* file) : file(file) {}
    void print(StringSlice string) override;
    void print(WideStringSlice string) override;
};

class DebugPrinter: public Printer {
public:
    void print(StringSlice string) override;
    void print(WideStringSlice string) override;
};

template<typename Char>
class StringPrinter: public Printer {
    GenericString<Char> output;
public:

    void print(StringSlice string) override {
        if constexpr(std::same_as<Char, char>) {
            output.append(string);
        } else {
            using_temp_allocator(
                WideString wide_string = string;
            )
            output.append(wide_string);
        }
    }

    void print(WideStringSlice string) override {
        if constexpr(std::same_as<Char, wchar_t>) {
            output.append(string);
        } else {
            using_temp_allocator(
                String narrow_string = string;
            )
            output.append(narrow_string);
        }
    }

    GenericString<Char> flush() {
        return std::move(output);
    }
};

extern StdFilePrinter stdout_printer;
extern StdFilePrinter stderr_printer;
extern DebugPrinter debug_printer;

namespace impl {

template<typename Char>
bool print_until_first_curlies(GenericStringSlice<Char> format, size_t* cur_chunk_begin) {
    enum class ParseState {
        Normal,
        SawOpenCurly,
        SawCloseCurly,
        End,
    };
    *cur_chunk_begin = 0;
    ParseState state = ParseState::Normal;
    for(auto i: format.indices()) {
        switch(state) {
            case ParseState::Normal: {
                if(format[i] == '{') {
                    state = ParseState::SawOpenCurly;
                } else if(format[i] == '}') {
                    state = ParseState::SawCloseCurly;
                }
                break;
            }

            case ParseState::SawOpenCurly: {
                if(format[i] == '{') {
                    zw_get_ctx(printer)->print(format[Range(*cur_chunk_begin, i)]);
                    *cur_chunk_begin = i + 1;
                } else if(format[i] == '}') {
                    zw_get_ctx(printer)->print(format[Range(*cur_chunk_begin, i-1)]);
                    *cur_chunk_begin = i + 1;
                    state = ParseState::End;
                    goto END;
                } else {
                    assert(false && "Invalid character in format string, expected curly brace");
                }
                break;
            }

            case ParseState::SawCloseCurly: {
                if(format[i] == '}') {
                    zw_get_ctx(printer)->print(format[Range(*cur_chunk_begin, i-1)]);
                    *cur_chunk_begin = i + 1;
                } else {
                    assert(false && "Invalid character in format string, expected curly brace");
                }
                break;
            }
        }
    }
    END:
    return state == ParseState::End;
}

template<typename Char>
void print(GenericStringSlice<Char> format) {
    zw_get_ctx(printer)->print(format);
    size_t cur_chunk_begin = 0;
    bool found_curlies = print_until_first_curlies(format, &cur_chunk_begin);
    assert(!found_curlies && "Too few arguments passed to fmt function");
}

template<typename Char, typename D0, typename... Ds>
void print(GenericStringSlice<Char> format, D0&& d0, Ds&&... ds) {
    size_t cur_chunk_begin = 0;
    bool found_curlies = impl::print_until_first_curlies(format, &cur_chunk_begin);
    assert(found_curlies && "Too many arguments passed to fmt function");
    ::zw_display(std::forward<D0>(d0));
    if(cur_chunk_begin < format.size()) {
        impl::print(format[Range(cur_chunk_begin, format.size())], std::forward<Ds>(ds)...);
    } else if(cur_chunk_begin > format.size()) {
        abort();
    }
}
};

struct Indentation {
    uint32_t amount;
    explicit Indentation(uint32_t amount = zw_get_ctx(indent)) : amount(amount) {}
};

};

// Narrow strings
template<typename... Ds>
void zw_print(zw::StringSlice format, Ds&&... ds) {
    zw::impl::print(format, std::forward<Ds>(ds)...);
}

template<typename... Ds>
zw::String zw_format(zw::StringSlice format, Ds&&... ds) {
    zw::StringPrinter<char> printer;
    zw_set_ctx(printer, &printer);
    zw_print(format, std::forward<Ds>(ds)...);
    return printer.flush();
}

template<typename... Ds>
zw::NoDestruct<zw::String> zw_temp_format(zw::StringSlice format, Ds&&... ds) {
    use_temp_allocator();
    return zw_format(format, std::forward<Ds>(ds)...);
}

// Wide strings
template<typename... Ds>
void zw_print(zw::WideStringSlice format, Ds&&... ds) {
    zw::impl::print(format, std::forward<Ds>(ds)...);
}

template<typename... Ds>
zw::WideString zw_format(zw::WideStringSlice format, Ds&&... ds) {
    zw::StringPrinter<wchar_t> printer;
    zw_set_ctx(printer, &printer);
    zw_print(format, std::forward<Ds>(ds)...);
    return printer.flush();
}

template<typename... Ds>
zw::NoDestruct<zw::WideString> zw_temp_format(zw::WideStringSlice format, Ds&&... ds) {
    use_temp_allocator();
    return zw_format(format, std::forward<Ds>(ds)...);
}

void zw_display(zw::StringSlice string);
void zw_display(zw::WideStringSlice string);
inline void zw_display(const char* string) {
    zw_display(zw::StringSlice(string));
}
inline void zw_display(const wchar_t* string) {
    zw_display(zw::WideStringSlice(string));
}
void zw_display(const zw::Indentation& indentation);
void zw_display(float val);

void zw_display(bool val);

void zw_display(std::unsigned_integral auto val) {
    char buf[512];
    if constexpr(sizeof(val) <= 4) {
        sprintf_s(buf, sizeof(buf), "%u", val);
    } else {
        sprintf_s(buf, sizeof(buf), "%llu", val);
    }
    zw_display(buf);
}

void zw_display(std::signed_integral auto val) {
    char buf[512];
    if constexpr(sizeof(val) <= 4) {
        sprintf_s(buf, sizeof(buf), "%d", val);
    } else {
        sprintf_s(buf, sizeof(buf), "%lld", val);
    }
    zw_display(buf);
}


template<zw::Display D>
void zw_display(const zw::Array<D>& array) {
    if(array.size() < 2) {
        zw_print("[");
        if(array.size()) {
            zw_display(array[0]);
        }
        zw_print("]");
    } else {
        zw_println("[");
        {
            zw_set_ctx(indent, zw_get_ctx(indent) + 4);
            for(auto i: array.indices()) {
                zw_println("{}{},", zw::Indentation(), array[i]);
            }
        }
        zw_print("{}]", zw::Indentation());
    }
}

template<zw::Display D>
void zw_display(const zw::NoDestruct<D>& object) {
    zw_display(*object);
}

#define ZW_DISPLAY_STRUCT_BEGIN(name) { ::zw_print(#name " {{\n"); }
#define ZW_DISPLAY_STRUCT_END() { ::zw_print("{}}}", ::zw::Indentation()); }
#define ZW_DISPLAY_FIELD(name) { \
    zw_set_ctx(indent, zw_get_ctx(indent) + 4); \
    ::zw_print("{}.{} = {},\n", ::zw::Indentation(), #name, val.name); \
}

#define ZW_DISPLAY_ENUM_BEGIN() switch(val) {
#define ZW_DISPLAY_ENUM_END(name) \
    default: \
        ::zw_print("{Unknown " #name " value}"); \
        break; \
    }
#define ZW_DISPLAY_ENUM_CASE(name) \
    case name: \
        ::zw_print(#name); \
        break

#define zw_dbg(expr) ([&]{ \
    zw_set_ctx(printer, &::zw::debug_printer); \
    auto val = expr; \
    ::zw_print("[" __FILE__ ":" ZW_STRINGIFY(__LINE__) "] " #expr " = {}\n", val); \
    return val; \
})()
