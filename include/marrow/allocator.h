#ifndef MARROW_ALLOCATOR_H
#define MARROW_ALLOCATOR_H

#include "marrow.h"

#include <string.h>

typedef void* (allocator_alloc_func)(void* ctx, usize size);
typedef void* (allocator_realloc_func)(void* ctx, void* ptr, usize old_size, usize new_size);
typedef void (allocator_free_func)(void* ctx, void* ptr, usize size);

typedef struct
{
    void* context;
    allocator_alloc_func* alloc;
    allocator_realloc_func* realloc;
    allocator_free_func* free;
} Allocator;

void* _default_alloc(void *ctx, usize size) { return malloc(size); }
void* _default_realloc(void* ctx, void* ptr, usize old_size, usize new_size) { return realloc(ptr, new_size); }
void _default_free(void* ctx, void* ptr, usize size) { free(ptr); }

#define DEFAULT_ALLOCATOR (Allocator) {.alloc = &_default_alloc, .realloc = &_default_realloc, .free = &_default_free }
// makes it default if its uninitialized
#define INIT_ALLOCATOR(allocator) if (!allocator.context && !allocator.alloc && !allocator.realloc && !allocator.free) allocator = DEFAULT_ALLOCATOR


void* allocator_alloc(Allocator* allocator, usize size)
{
    return allocator->alloc(allocator->context, size);
}

void* allocator_make_copy(Allocator* allocator, void* ptr, usize size)
{
    void* new_ptr = allocator_alloc(allocator, size);
    memcpy(new_ptr, ptr, size);
    return new_ptr;
}

void* allocator_realloc(Allocator* allocator, void* ptr, usize old_size, usize new_size)
{
    return allocator->realloc(allocator->context, ptr, old_size, new_size);
}

void allocator_free(Allocator* allocator, void* ptr, usize size)
{
    allocator->free(allocator->context, ptr, size);
}

// LINEAR ALLOCATOR
// simple linear allocator, doesn't grow and doesnt free anything
typedef struct { void *data; usize data_size; usize ptr; } LinearAllocator;

void *linear_allocator_alloc(void *ctx, usize size)
{
    LinearAllocator* allocator = (LinearAllocator*)ctx;
    void* ret = allocator->ptr + size <= allocator->data_size ? (u8*)allocator->data + allocator->ptr : nullptr;
    allocator->ptr += size;
    return ret;
}

void linear_allocator_free(void* ctx, void* ptr, usize size) { return; }

// LINEAR ALLOCATOR


#endif // MARROW_ALLOCATOR_H
