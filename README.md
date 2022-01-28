# ZW: A public-domain general-purpose support library for C++

## How to build

ZW uses the [ABS](https://github.com/zachwolfe/abs) build system. To try ZW with ABS, install ABS and clone ZW, create a new ABS project with `abs init project_name`, then add the path to ZW to the `dependencies` array in your `abs.json` file.

## Supported platforms

Only Windows, for now.

## Features:

### Context system

Inspired by newer languages like Jai and Odin, ZW supports a context system through which values will be implicitly passed down the stack, and can be modified locally without affecting the caller. The set of values that can be passed is fixed at compile-time in a struct in the `context.h` file. To get or set a context value, use the `zw_get_ctx` and `zw_set_ctx` macros. Under the hood, this is implemented using a combination of thread local variables and RAII.

Example:
```cpp
#include <stdio.h>
#include <zw/context.h> // must be modified to include fib_argument

int fib() {
    if(zw_get_ctx(fib_argument) < 2) {
        return zw_get_ctx(fib_argument);
    } else {
        int result;
        {
            // This value only applies to the current scope
            zw_set_ctx(fib_argument, zw_get_ctx(fib_argument) - 1);
            result = fib();
        }
        // fib_argument has now been reset to what it was at the beginning of the call
        zw_set_ctx(fib_argument, zw_get_ctx(fib_argument) - 2);
        result += fib();
        return result;
    }
}

int main() {
    zw_set_ctx(fib_argument, 7);
    printf("fib(7) = %d\n", fib()); // fib(7) = 13
    return 0;
}
```

### Custom allocators

```cpp
#include <zw/alloc.h>

int main() {
    int* num = (int*)zw_alloc(/*size*/sizeof(int), /*alignment*/alignof(int)); // Calls malloc
    *num = 25;
    zw_free(num); // Calls free

    zw::InlineAllocator<2500> inline_allocator;
    {
        zw_set_ctx(allocator, &inline_allocator);
        int* num_on_the_stack = (int*)zw_alloc(sizeof(int), alignof(int)); // Calls inline_allocator's alloc method
        *num_on_the_stack = 25;
        zw_free(num_on_the_stack); // Does nothing
    }
    
    return 0;
}
```

#### Sub-feature: allocator identity checking

Especially when using automatic memory management via RAII (see below), I found it to be very easy to accidentally end up freeing an allocation with a different allocator than was originally used to create it. To mitigate this problem, when the `ZW_ALLOC_SAFETY` macro is defined (which it is by default), the included allocators store some extra information with every allocation to uniquely identify the allocator that it belongs to (including across threads). Every `realloc` or `free` will assert that the passed in allocation matches the allocator's unique ID.

Example:

```cpp
#include <zw/alloc.h>

int main() {
    int* num = (int*)zw_alloc(sizeof(int), alignof(int)); // Calls malloc
    *num = 25;
    zw::InlineAllocator<1024> inline_allocator;
    zw_set_ctx(allocator, &inline_allocator);
    zw_free(num); // Assertion will fail here
    
    return 0;
}
```

### ZwObject: prevention of implicit copies

When the `ZW_AUDIT_IMPLICIT_COPIES` is defined (which it is by default), if a class inherits from ZwObject and calls ZwObject's copy constructor its own copy constructor and copy assignment operator (directly or indirectly) will fail to complete unless explicitly performed using the `gt_copy()` or `gt_copy_to()` functions.

Example:

```cpp
#include <zw/alloc.h>

#define LARGE_NUMBER 1000000000
class ExpensiveToCopy : ZwObject {
    void* data;

public:
    ExpensiveToCopy() : data(zw_alloc(LARGE_NUMBER, 1)) {
        memset(data, 0, LARGE_NUMBER);
    }
    ExpensiveToCopy(const ExpensiveToCopy& other) : ZwObject(other) {
        data = zw_alloc(LARGE_NUMBER, 1);
        memcpy(data, other.data, LARGE_NUMBER);
    }
    ExpensiveToCopy& operator=(const ExpensiveToCopy& other) {
        if(this != &other) {
            // Calls copy constructor above
            ExpensiveToCopy temp = other;
            std::swap(*this, temp);
        }
        return *this;
    }

    ExpensiveToCopy& operator=(ExpensiveToCopy&& other) {
        if(this != &other) {
            data = other.data;
            other.data = nullptr;
        }
        return *this;
    }

    ~ExpensiveToCopy() {
        if(data) {
            zw_free(data);
        }
    }
};

int main() {
    ExpensiveToCopy expensive;
    // Assertion would fail here:
    //   ExpensiveToCopy copy = expensive;
    // As it would here:
    //   ExpensiveToCopy copy;
    //   copy = expensive;

    // But not here:
    ExpensiveToCopy allowed_copy = zw_copy(expensive);

    // Nor here
    zw_copy_to(&expensive, allowed_copy);

    return 0;
}
```


### std::vector-like growable Array data structure

TODO: fill in this section

### String and WideString

TODO: fill in this section

### Formatting

TODO: fill in this section