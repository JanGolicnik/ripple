#ifndef MARROW_ALLOCATOR_H
#define MARROW_ALLOCATOR_H

#include "marrow.h"

typedef void* (allocator_alloc_func)(void* allocator, usize size, usize align);
typedef void* (allocator_realloc_func)(void* allocator, void* ptr, usize old_size, usize new_size, usize align);
typedef void (allocator_free_func)(void* allocator, void* ptr, usize size);

typedef struct
{
    void* context;
    allocator_alloc_func* alloc;
    allocator_realloc_func* realloc;
    allocator_free_func* free;
} Allocator;

void* _default_alloc(void *ctx, usize size, usize align) { return malloc(size); }
void* _default_realloc(void* ctx, void* ptr, usize old_size, usize new_size, usize align) { return realloc(ptr, new_size); }
void _default_free(void* ctx, void* ptr, usize size) { free(ptr); }

thread_local static Allocator _default_allocator = {.alloc = &_default_alloc, .realloc = &_default_realloc, .free = &_default_free };
Allocator* default_allocator() { return &_default_allocator; }

static bool allocator_debug = true;

void* allocator_alloc(Allocator* allocator, usize size, usize align)
{
    if (allocator_debug)
    {
        debug("allocating {}", size);
    }
    allocator = allocator ? allocator : default_allocator();
    return allocator->alloc(allocator, size, align);
}

void* allocator_make_copy(Allocator* allocator, void* ptr, usize size, usize align)
{
    void* new_ptr = allocator_alloc(allocator, size, align);
    buf_copy(new_ptr, ptr, size);
    return new_ptr;
}

void* allocator_realloc(Allocator* allocator, void* ptr, usize old_size, usize new_size, usize align)
{
    allocator = allocator ? allocator : default_allocator();
    return allocator->realloc(allocator, ptr, old_size, new_size, align);
}

#define allocator_alloc_t(alloc, T) \
    ((T*) allocator_alloc((alloc), sizeof(T), alignof(T)))

#define allocator_array_t(alloc, T, count) \
    ((T*) allocator_alloc((alloc), sizeof(T) * (count), alignof(T)))

#define allocator_realloc_t(alloc, ptr, old_count, new_count, T) \
    ((T*) allocator_realloc((alloc), (ptr), \
        sizeof(T) * (old_count), sizeof(T) * (new_count), alignof(T)))

#define allocator_make_copy_t(alloc, src, count, T) \
    ((T*) allocator_make_copy((alloc), (src), sizeof(T) * (count), alignof(T)))

void allocator_free(Allocator* allocator, void* ptr, usize size)
{
    allocator = allocator ? allocator : default_allocator();
    allocator->free(allocator, ptr, size);
}

static inline void* align_up(void* x, size_t a) {
    return (void*)(((usize)x + (a-1)) & ~(uintptr_t)(a-1));
}

// BUMP ARENA
typedef struct BumpAllocatorBlock {
    struct BumpAllocatorBlock* next;
    usize capacity, used;
    u8 data[];
} BumpAllocatorBlock;

typedef struct {
    Allocator allocator;
    BumpAllocatorBlock* first, *last;
    Allocator* inner_allocator;
} BumpAllocator;

static void* bump_allocator_alloc(void* a, usize size, usize align);

static BumpAllocatorBlock* bump_allocator_block_new(usize capacity, Allocator* allocator)
{
    BumpAllocatorBlock* block = (BumpAllocatorBlock*)allocator_alloc(allocator, sizeof(BumpAllocatorBlock) + capacity, 1);
    block->next = nullptr;
    block->capacity = capacity;
    block->used = 0;
    return block;
}

static void* bump_allocator_alloc(void* allocator, usize size, usize align)
{
    BumpAllocator* a = allocator;

    if (!a->last) {
        a->last = a->first = bump_allocator_block_new(1024, a->inner_allocator);
    }

    BumpAllocatorBlock* b = a->last;
    loop {
        u8* ptr = b->data + b->used;
        u8* aligned_ptr = align_up(ptr, align);
        usize used = (usize)(aligned_ptr - b->data) + size;

        if (used < b->capacity)
        {
            b->used = used;
            a->last = b;
            return aligned_ptr;
        }

        if (!b->next)
            b->next = bump_allocator_block_new(b->capacity * 2, a->inner_allocator);

        b = b->next;
    }
}

static void bump_allocator_free(void* allocator, void* ptr, usize size)
{
    return;
}

BumpAllocator bump_allocator_create()
{
    return (BumpAllocator) {
        .allocator.alloc = bump_allocator_alloc,
        .allocator.free = bump_allocator_free,
    };
}

void bump_allocator_reset(BumpAllocator* a)
{
    for (BumpAllocatorBlock* b = a->first; b; b = b->next) b->used = 0;
    a->last = a->first;
}

#endif // MARROW_ALLOCATOR_H
