#include <stdlib.h>

#include "alloc.h"
#include "context.h"
#include "misc.h"
#include "fmt.h"

void* zw_alloc(zw::Allocator* allocator, size_t size, size_t alignment) {
    return allocator->alloc(size, alignment);
}
void zw_free(zw::Allocator* allocator, void* address) {
    allocator->free(address);
}
void* zw_realloc(zw::Allocator* allocator, void* address, size_t size, size_t alignment) {
    return allocator->realloc(address, size, alignment);
}
void zw_alloc_reset(zw::Allocator* allocator) {
    allocator->reset();
}

void* zw_alloc(size_t size, size_t alignment) {
    return zw_get_ctx(allocator)->alloc(size, alignment);
}
void zw_free(void* address) {
    zw_get_ctx(allocator)->free(address);
}
void* zw_realloc(void* address, size_t size, size_t alignment) {
    return zw_get_ctx(allocator)->realloc(address, size, alignment);
}
void zw_alloc_reset() {
    zw_get_ctx(allocator)->reset();
}

void* zw_temp_alloc(size_t size, size_t alignment) {
    return zw_get_ctx(temp_allocator)->alloc(size, alignment);
}
void zw_temp_free(void* address) {
    zw_get_ctx(temp_allocator)->free(address);
}
void* zw_temp_realloc(void* address, size_t size, size_t alignment) {
    return zw_get_ctx(temp_allocator)->realloc(address, size, alignment);
}
void zw_temp_alloc_reset() {
    zw_get_ctx(temp_allocator)->reset();
}

namespace zw {

static uint32_t get_thread_id() {
    // TODO: cross-platform
    return CoGetCurrentProcess();
}

GlobalAllocator global_allocator {};
extern thread_local InlineAllocator<DEFAULT_TEMP_ALLOCATOR_SIZE> temp_allocator {};

// Global allocator takes up one spot
static thread_local uint32_t allocator_count = 1;

struct AllocationHeader {
    void* base;
    size_t size;
#ifdef ZW_ALLOC_SAFETY
    uint32_t allocator_id;
    uint32_t thread_id;
#endif
};

// Allocator
void Allocator::reset() {
    abort();
}

template<typename Address>
AllocationHeader* find_header(Address address) {
    return (AllocationHeader*)((uintptr_t)address - sizeof(AllocationHeader));
}

// GlobalAllocator
void* GlobalAllocator::alloc(size_t size, size_t alignment) {
    void* base = malloc(size + sizeof(AllocationHeader) + alignment);
    if(base) {
        uintptr_t addr = (uintptr_t)base;
        addr += sizeof(AllocationHeader);
        addr = nearest_multiple_of(addr, alignment);
        AllocationHeader* header = find_header(addr);
        header->base = base;
        header->size = size;
        #ifdef ZW_ALLOC_SAFETY
        header->allocator_id = 0;
        header->thread_id = 0; // Irrelevant to global allocator
        #endif
        return (void*)addr;
    } else {
        return nullptr;
    }
}
void* GlobalAllocator::realloc(void* address, size_t size, size_t alignment) {
    if(!address) return alloc(size, alignment);

    AllocationHeader* header = find_header(address);
    #ifdef ZW_ALLOC_SAFETY
    assert(header->allocator_id == 0);
    #endif
    uintptr_t original_offset = (uintptr_t)address - (uintptr_t)header->base;
    void* original_base = header->base;
    size_t original_size = header->size;
    void* new_base = ::realloc(header->base, size + sizeof(AllocationHeader) + alignment);
    if(!new_base) return nullptr;
    if(new_base == original_base) {
        header->size = size;
        return address;
    } else {
        // Uh-oh, realloc had to move to another location. That might have messed with the alignment.
        uintptr_t addr = (uintptr_t)new_base;
        addr += sizeof(AllocationHeader);
        addr = nearest_multiple_of(addr, alignment);

        // Fix-up alignment
        uintptr_t new_offset = addr - (uintptr_t)new_base;
        if(new_offset != original_offset) {
            memmove((void*)addr, (uint8_t*)new_base + original_offset, original_size);
        }

        AllocationHeader* header = find_header(addr);
        header->base = new_base;
        header->size = size;
        #ifdef ZW_ALLOC_SAFETY
        header->allocator_id = 0;
        header->thread_id = 0; // Irrelevant to global allocator
        #endif

        return (void*)addr;
    }
}

void GlobalAllocator::free(void* address) {
    AllocationHeader* header = find_header(address);
    #ifdef ZW_ALLOC_SAFETY
    assert(header->allocator_id == 0);
    #endif
    ::free(header->base);
}

// LinearAllocator

#ifdef ZW_ALLOC_SAFETY
LinearAllocator::LinearAllocator(uint8_t* buffer, size_t size) : Allocator(allocator_count++, get_thread_id()), buffer(buffer), size(size) {}
#endif

void* LinearAllocator::alloc(size_t size, size_t alignment) {
    uintptr_t address = (uintptr_t)buffer + bump;
    address += sizeof(AllocationHeader);
    address = nearest_multiple_of(address, alignment);
    uintptr_t padded_size = address + size - (uintptr_t)(buffer + bump);
    if(bump + padded_size <= this->size) {
        AllocationHeader* header = find_header(address);
        header->base = nullptr; // Not meaningful for the linear allocator
        header->size = size;
        #ifdef ZW_ALLOC_SAFETY
        header->allocator_id = allocator_id;
        header->thread_id = thread_id;
        #endif
        bump += padded_size;
        previous_allocation = (void*)address;
        return (void*)address;
    } else {
        return nullptr;
    }
}
void* LinearAllocator::realloc(void* address, size_t size, size_t alignment) {
    if(!address) return alloc(size, alignment);

    if(previous_allocation == address) {
        uintptr_t addr = (uintptr_t)address;
        uintptr_t new_bump = addr - (uintptr_t)buffer + size;
        if(addr - (uintptr_t)buffer + size <= this->size) {
            bump = new_bump;
            previous_allocation = address;
            AllocationHeader* header = find_header(addr);
            header->size = size;
            return address;
        } else {
            return nullptr;
        }
    } else {
        uintptr_t addr = (uintptr_t)address;
        AllocationHeader* header = find_header(addr);

        #ifdef ZW_ALLOC_SAFETY
        if(header->allocator_id != allocator_id) {
            zw_dbg(header->allocator_id);
            zw_dbg(allocator_id);
        }
        assert(header->allocator_id == allocator_id);
        assert(header->thread_id == thread_id);
        #endif

        // There's nothing I can do in this case other than allocate a totally new buffer.
        void* new_allocation = alloc(size, alignment);
        if(!new_allocation) return nullptr;

        memcpy(new_allocation, address, header->size);
        return new_allocation;
    }
}
void LinearAllocator::free(void* address) {
    #ifdef ZW_ALLOC_SAFETY
    AllocationHeader* header = find_header(address);
    assert(header->allocator_id == allocator_id);
    assert(header->thread_id == thread_id);
    #endif
}
void LinearAllocator::reset() {
    bump = 0;
    previous_allocation = 0;
}

};