#ifndef MARROW_VEKTOR_H
#define MARROW_VEKTOR_H

#include "marrow.h"
#include "allocator.h"

#include <string.h>

typedef size_t vektor_size_t;

#ifdef MARROW_VEKTOR_IMPLEMENTATION

typedef struct
{
  void* data;
  vektor_size_t element_size;
  vektor_size_t size;
  vektor_size_t capacity;
  Allocator* allocator;
} Vektor;

#else

typedef void Vektor;

#endif

Vektor* vektor_create(vektor_size_t initial_capacity, vektor_size_t element_size, Allocator* allocator);
bool vektor_init(Vektor* vektor, vektor_size_t initial_capacity, vektor_size_t element_size, Allocator* allocator);
bool vektor_destroy(Vektor** vektor);
bool vektor_empty(Vektor* vektor); // keeps capacity
bool vektor_clear(Vektor* vektor); // deallocates

void* vektor_add(Vektor* vektor, const void* data);
void* vektor_insert(Vektor* vektor, vektor_size_t location, const void* data);

vektor_size_t vektor_size(Vektor* vektor);
void vektor_set_size(Vektor* vektor, vektor_size_t new_size);

void* vektor_get(Vektor* vektor, vektor_size_t index);
void* vektor_pop(Vektor* vektor);
void* vektor_last(Vektor* vektor);
bool vektor_remove(Vektor* vektor, vektor_size_t index);

#ifdef MARROW_VEKTOR_IMPLEMENTATION
#undef MARROW_VEKTOR_IMPLEMENTATION

Vektor* vektor_create(vektor_size_t initial_capacity, vektor_size_t element_size, Allocator* allocator)
{
    Vektor* vektor = allocator ? allocator->alloc(allocator->context, sizeof(Vektor)) : malloc(sizeof(Vektor));
    vektor_init(vektor, initial_capacity, element_size, allocator);
    return vektor;
}

bool vektor_init(Vektor* vektor, vektor_size_t initial_capacity, vektor_size_t element_size, Allocator* allocator)
{
    usize alloc_size = initial_capacity * element_size;
    *vektor = (Vektor){
      .data = initial_capacity ?
                allocator ? allocator->alloc(allocator->context, alloc_size) : malloc(alloc_size)
                : nullptr,
      .size = 0,
      .capacity = initial_capacity,
      .element_size = element_size,
      .allocator = allocator,
    };
    return true;
}

bool vektor_destroy(Vektor** vektor_ptr)
{
    if(!vektor_ptr)
        return false;

    Vektor* vektor = *vektor_ptr;
    vektor_clear(vektor);
    vektor->allocator ? vektor->allocator->free(vektor->allocator->context, vektor, sizeof(Vektor)) : free(vektor);

    *vektor_ptr = nullptr;
    return true;
}

bool vektor_empty(Vektor* vektor)
{
    vektor->size = 0;
    return true;
}

bool vektor_clear(Vektor* vektor)
{
    usize alloc_size = vektor->capacity * vektor->element_size;
    vektor->allocator ? vektor->allocator->free(vektor->allocator->context, vektor->data, alloc_size) : free(vektor->data);
    vektor->size = 0;
    vektor->capacity = 0;
    vektor->data = nullptr;

    return true;
}

void internal_vektor_grow(Vektor* vektor)
{
    usize old_alloc_size = vektor->capacity * vektor->element_size;
    vektor->capacity = vektor->capacity * 1.5 + 1;
    usize alloc_size = vektor->capacity * vektor->element_size;

    if (!vektor->data) {
        vektor->data = vektor->allocator ?
            vektor->allocator->alloc(vektor->allocator->context, alloc_size) : malloc(alloc_size);
        return;
    }

    vektor->data = vektor->allocator ?
        vektor->allocator->realloc(vektor->allocator->context, vektor->data, alloc_size, old_alloc_size) :
        realloc(vektor->data, alloc_size);
}

void* vektor_add(Vektor* vektor, const void* data)
{
    if (vektor->size >= vektor->capacity)
        internal_vektor_grow(vektor);

    memcpy((u8*)vektor->data + vektor->size * vektor->element_size, data, vektor->element_size);
    vektor->size++;
    return vektor_last(vektor);
}

vektor_size_t vektor_size(Vektor* vektor)
{
    return vektor->size;
}

void vektor_set_size(Vektor* vektor, vektor_size_t new_size)
{
    vektor->size = new_size;
    while(vektor->size > vektor->capacity)
        internal_vektor_grow(vektor);
}

void* vektor_insert(Vektor* vektor, vektor_size_t location, const void* data)
{
    if (vektor->size == location)
        return vektor_add(vektor, data);

    while(vektor->size >= vektor->capacity)
        internal_vektor_grow(vektor);

    location *= vektor->element_size;
    u8* destination = (u8*)vektor->data + location;

    memcpy(destination,
           destination + vektor->element_size,
           vektor->size * vektor->element_size - location);

    memcpy(destination, data, vektor->element_size);

    return destination;
}

void* vektor_get(Vektor* vektor, vektor_size_t index)
{
    if (index >= vektor->size)
    {
        error("accessing element out of vektor range");
        return nullptr;
    }
    return (u8*)vektor->data + index * vektor->element_size;
}

void* vektor_pop(Vektor* vektor)
{
    if (vektor->size == 0) return nullptr;

    vektor->size--;
    return (u8*)vektor->data + vektor->size * vektor->element_size;
}

void* vektor_last(Vektor* vektor)
{
    if (vektor->size == 0) return nullptr;
    return (u8*)vektor->data + (vektor->size - 1) * vektor->element_size;
}

bool vektor_remove(Vektor* vektor, vektor_size_t index)
{
    i64 n = vektor->size - 1 - index;
    if (n <= 0)
        return true;

    index *= vektor->element_size;
    memcpy((u8*)vektor->data + index,
           (u8*)vektor->data + index + vektor->element_size,
            n * vektor->element_size);
    return true;
}

#endif // MARROW_VEKTOR_IMPLEMENTATION

#endif // MARROW_VEKTOR_H
