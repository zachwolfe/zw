#pragma once

#include <assert.h>
#include <stdint.h>
#include <utility>

#include "context.h"

namespace zw { class Allocator; };

ZW_DECLARE_CTX_VAR(zw::Allocator*, allocator);
ZW_DECLARE_CTX_VAR(zw::Allocator*, temp_allocator);
ZW_DECLARE_CTX_VAR(bool, is_explicitly_copying);

#define use_temp_allocator() zw_set_ctx(allocator, zw_get_ctx(temp_allocator))
#define using_temp_allocator(code) \
    auto ZW_CONCAT(__allocator_old_value__, __LINE__) = zw_get_ctx(allocator); \
    zw_set_ctx(allocator, zw_get_ctx(temp_allocator), first); \
    code; \
    zw_set_ctx(allocator, ZW_CONCAT(__allocator_old_value__, __LINE__), second);

#define ZW_ALLOC_SAFETY
#define ZW_AUDIT_IMPLICIT_COPIES

// These are the main entry points to the allocator API

void* zw_alloc(zw::Allocator* allocator, size_t size, size_t alignment);
void zw_free(zw::Allocator* allocator, void* address);
void* zw_realloc(zw::Allocator* allocator, void* address, size_t size, size_t alignment);
void zw_alloc_reset(zw::Allocator* allocator);

void* zw_alloc(size_t size, size_t alignment);
void zw_free(void* address);
void* zw_realloc(void* address, size_t size, size_t alignment);
void zw_alloc_reset();

void* zw_temp_alloc(size_t size, size_t alignment);
void zw_temp_free(void* address);
void* zw_temp_realloc(void* address, size_t size, size_t alignment);
void zw_temp_alloc_reset();

template<typename Value>
Value* zw_make() {
    void* address = zw_alloc(sizeof(Value), alignof(Value));
    if(address) {
        return new(address) Value();
    } else {
        return nullptr;
    }
}

template<typename Value, typename Param0, typename... Params>
Value* zw_make(Param0&& param0, Params&&... params) {
    void* address = zw_alloc(sizeof(Value), alignof(Value));
    if(address) {
        return new(address) Value(std::forward<Param0>(param0), std::forward<Params>(params)...);
    } else {
        return nullptr;
    }
}

template<typename Value>
void zw_destroy(Value* value) {
    if(value) {
        value->~Value();
        zw_free(value);
    }
}

template<typename Value>
Value* zw_temp_make() {
    use_temp_allocator();
    return zw_make<Value>();
}

template<typename Value, typename Param0, typename... Params>
Value* zw_temp_make(Param0&& param0, Params&&... params) {
    use_temp_allocator();
    return zw_make<Value>(std::forward<Param0>(param0), std::forward<Params>(params)...);
}

template<typename Value>
void zw_temp_destroy(Value* value) {
    use_temp_allocator();
    zw_destroy(value);
}

template<typename Value>
Value* zw_make_with_alloc(zw::Allocator* allocator) {
    zw_set_ctx(allocator, allocator);
    return zw_make<Value>();
}

template<typename Value, typename Param0, typename... Params>
Value* zw_make_with_alloc(zw::Allocator* allocator, Param0&& param0, Params&&... params) {
    zw_set_ctx(allocator, allocator);
    return zw_make<Value>(std::forward<Param0>(param0), std::forward<Params>(params)...);
}

template<typename Value>
void zw_destroy_with_alloc(zw::Allocator* allocator, Value* value) {
    zw_set_ctx(allocator, allocator);
    zw_destroy(value);
}

struct ZwObject {
    #ifdef ZW_AUDIT_IMPLICIT_COPIES
    ZwObject() = default;
    ZwObject(const ZwObject& other) {
        assert(zw_get_ctx(is_explicitly_copying));
    }
    #endif
};

template<typename T>
void zw_copy_to(T* location, const T& value) {
    #ifdef ZW_AUDIT_IMPLICIT_COPIES
    zw_set_ctx(is_explicitly_copying, true);
    #endif

    *location = value;
}

template<typename T>
T zw_copy(const T& value) {
    #ifdef ZW_AUDIT_IMPLICIT_COPIES
    zw_set_ctx(is_explicitly_copying, true);
    #endif
    return value;
}

namespace zw {

class Allocator {
#ifdef ZW_ALLOC_SAFETY
protected:
    // These members uniquely identify an allocator during the entire run of the program.
    // allocator_id is the index of the allocator local to the thread it was created on,
    // and the thread id enables one to distinguish between allocators with the same ID created
    // on different threads.
    // All GlobalAllocator instances share the values 0, 0. It is incorrect for any other allocator
    // to claim an allocator_id of 0.
    uint32_t allocator_id, thread_id;
    Allocator(uint32_t allocator_id, uint32_t thread_id) : allocator_id(allocator_id), thread_id(thread_id) {}

    void check_header(void* address);
#endif

public:
    virtual void* alloc(size_t size, size_t alignment) = 0;
    virtual void free(void* address) = 0;
    virtual void* realloc(void* address, size_t size, size_t alignment) = 0;

    // The default implementation just aborts, since most allocators cannot be reset in one go.
    virtual void reset();
};

class GlobalAllocator: public Allocator {
public:
#ifdef ZW_ALLOC_SAFETY
    GlobalAllocator() : Allocator(0, 0) {}
#endif
    void* alloc(size_t size, size_t alignment) override;
    void free(void* address) override;
    void* realloc(void* address, size_t size, size_t alignment) override;
};

class LinearAllocator: public Allocator {
protected:
    uint8_t* buffer;
    size_t bump = 0;
    size_t size = 0;
    void* previous_allocation = nullptr;
public:
#ifdef ZW_ALLOC_SAFETY
    LinearAllocator(uint8_t* buffer, size_t size);
#else
    LinearAllocator(uint8_t* buffer, size_t size) : buffer(buffer), size(size) {}
#endif

    void* alloc(size_t size, size_t alignment) override;
    void free(void* address) override;
    void* realloc(void* address, size_t size, size_t alignment) override;

    void reset() override;
};

template<size_t Size>
class InlineAllocator: public LinearAllocator {
    uint8_t storage[Size];
public:
    InlineAllocator() : LinearAllocator(storage, Size) {}

    InlineAllocator(const InlineAllocator<Size>& other) = delete;
    InlineAllocator(InlineAllocator<Size>&& other) = delete;
};

class ArenaAllocator: public LinearAllocator {
protected:
    size_t block_size;
    size_t block_alignment;
    void* first_free_block;
    void init();
public:
    ArenaAllocator(uint8_t* buffer, size_t buffer_size, size_t block_size, size_t block_alignment) : LinearAllocator(buffer, buffer_size), block_size(block_size), block_alignment(block_alignment), first_free_block(first_free_block) {
        init();
    }

    void* alloc(size_t size, size_t alignment) override;
    void free(void* address) override;
    void* realloc(void* address, size_t size, size_t alignment) override;

    void reset() override;
};

template<size_t Size>
class InlineArenaAllocator: public ArenaAllocator {
    uint8_t storage[Size];
public:
    InlineArenaAllocator(size_t block_size, size_t block_alignment) : ArenaAllocator(storage, Size, block_size, block_alignment) {}

    InlineArenaAllocator(const InlineArenaAllocator<Size>& other) = delete;
    InlineArenaAllocator(InlineArenaAllocator<Size>&& other) = delete;
};

template<typename Wrapped>
class alignas(alignof(Wrapped)) NoDestruct {
    union {
        Wrapped _object;
        char _data[sizeof(Wrapped)];
    };

    NoDestruct() {
        memset(_data, 0, sizeof(Wrapped));
    }
public:
    NoDestruct(Wrapped&& object) {
        new(&_object) Wrapped(std::move(object));
    }

    NoDestruct(NoDestruct<Wrapped>&& other) {
        new(&_object) Wrapped(std::move(other.take()));
        memset(&other._object, 0, sizeof(Wrapped));
    }

    NoDestruct(const NoDestruct<Wrapped>& other) {
        new(&_object) Wrapped(other._object);
    }

    NoDestruct& operator=(NoDestruct<Wrapped>&& other) {
        if(this != &other) {
            new(&_object) Wrapped(std::move(other.take()));
            memset(&other._object, 0, sizeof(Wrapped));
        }
        return *this;
    }

    NoDestruct& operator=(const NoDestruct<Wrapped>& other) {
        if(this != &other) {
            new(&_object) Wrapped(&other._object);
        }
        return *this;
    }

    static NoDestruct undefined() {
        return NoDestruct();
    }

    Wrapped* operator->() {
        return &_object;
    }

    const Wrapped& operator*() const {
        return _object;
    }
    Wrapped& operator*() {
        return _object;
    }

    Wrapped&& take() {
        return std::move(_object);
    }

    ~NoDestruct() {}
};

constexpr size_t DEFAULT_TEMP_ALLOCATOR_SIZE = 10000;
extern thread_local InlineAllocator<DEFAULT_TEMP_ALLOCATOR_SIZE> temp_allocator;
extern GlobalAllocator global_allocator;
};