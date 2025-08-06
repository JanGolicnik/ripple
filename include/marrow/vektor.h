#ifndef MARROW_VEKTOR_H
#define MARROW_VEKTOR_H

#include "marrow.h"
#include "allocator.h"

#define VEKTOR(item)\
struct \
{ \
    item* items; \
    u64 n_items; \
    u64 size; \
    Allocator* _allocator; \
}

#define vektor_init(v, initial_size, allocator) \
do { \
    v.size = initial_size; v.n_items = 0; v._allocator = allocator; \
    if (!v.size) break; \
    v.items = allocator_alloc(v._allocator, sizeof(*v.items) * v.size); \
    buf_set(v.items, 0, v.size * sizeof(*v.items)); \
} while (0)

#define vektor_free(v) \
do { \
    allocator_free(v._allocator, v.items, v.size * sizeof(*v.items)); \
    v.n_items = 0; v.size = 0; v.items = nullptr; \
} while (0)

#define vektor_clear(v) \
do { \
    v.n_items = 0;\
} while (0)

#define vektor_add(v, ...) vektor_insert(v, v.n_items, __VA_ARGS__)

#define vektor_insert(v, position, ...) \
do { \
    while (max(position, v.n_items) >= v.size) \
    { \
        u64 old_alloc_size = v.size * sizeof(*v.items); \
        v.size = v.size * 2 + 1; \
        v.items = v.items ? \
            allocator_realloc(v._allocator, v.items, old_alloc_size, v.size * sizeof(*v.items)) : \
            allocator_alloc(v._allocator, v.size * sizeof(*v.items)); \
    } \
    for (u64 i = v.n_items; i > position; i--) \
        v.items[i] = v.items[i - 1]; \
    v.items[position] = (__VA_ARGS__); \
    v.n_items++;\
} while (0)

#define vektor_remove(v, position) \
do { \
    if (position >= v.n_items) break; \
    for (u64 i = position; i < v.n_items - 1; i++) \
        v.items[i] = v.items[i + 1]; \
    v.n_items--; \
} while (0)

#endif // MARROW_VEKTOR_H
