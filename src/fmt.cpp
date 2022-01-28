#include <stdio.h>

#include "fmt.h"
#include "context.h"
#include "assert.h"

using namespace zw;

namespace zw {

extern StdFilePrinter stdout_printer{stdout};
extern StdFilePrinter stderr_printer{stderr};
extern DebugPrinter debug_printer{};

void StdFilePrinter::print(StringSlice string) {
    fprintf(file, "%.*s", (uint32_t)string.size, string.data);
}

void StdFilePrinter::print(WideStringSlice string) {
    fprintf(file, "%.*S", (uint32_t)string.size, string.data);
}

void DebugPrinter::print(StringSlice string) {
    auto null_terminated = string.c_string();
    OutputDebugStringA(null_terminated->data());
}

void DebugPrinter::print(WideStringSlice string) {
    auto null_terminated = string.c_string();
    OutputDebugStringW(null_terminated->data());
}

namespace impl {

enum class TokenKind {
    Slice,
    Curlies,
};
struct Token {
    TokenKind kind;
    StringSlice slice;
};

template<size_t N>
struct StaticString {
    char data[N];
    static constexpr size_t num = N;
    constexpr StaticString(const char (&string)[N]) {
        for(size_t i = 0; i < N; i++) {
            data[i] = string[i];
        }
    }
};

template<size_t N, StaticString<N> string>
constexpr size_t take_static_string() {
    return find_first_curlies(string.data).size();
}


};

// void aah() {
//     // constexpr impl::StaticString format_string = "Hello, {}. My name is {} and I like.\n";
//     // // size_t num = impl::take_static_string<decltype(format_string)::num, format_string>();
//     // constexpr 
//     constexpr size_t size = [] {return impl::tokenize_format("Hello, {}. My name is {} and I like.\n").size();}();
//     impl::Token tokens[size] = {};
//     // static_assert(num == 0);
// }

};

void zw_display(StringSlice string) {
    zw_get_ctx(printer)->print(string);
}

void zw_display(WideStringSlice string) {
    zw_get_ctx(printer)->print(string);
}

void zw_display(const zw::Indentation& indentation) {
    for(uint32_t i = 0; i < indentation.amount; i++) {
        zw_display(" ");
    }
}
void zw_display(float val) {
    char buf[512];
    sprintf_s(buf, sizeof(buf), "%0.6f", val);
    zw_display(buf);
}
void zw_display(bool val) {
    if(val) {
        zw_display("true");
    } else {
        zw_display("false");
    }
}