#ifndef MARROW_VEKTOR_H
#define MARROW_VEKTOR_H

#include "marrow.h"

typedef u32 vektor_size_t;

#ifdef MARROW_VEKTOR_IMPLEMENTATION

typedef struct
{
  void* data;
  vektor_size_t element_size;
  vektor_size_t size;
  vektor_size_t capacity;
} Vektor;

#else

typedef void Vektor;

#endif

Vektor* vektor_create(vektor_size_t initial_capacity, vektor_size_t element_size);
bool vektor_init(Vektor* vektor, vektor_size_t initial_capacity, vektor_size_t element_size);
bool vektor_destroy(Vektor** vektor);
bool vektor_empty(Vektor* vektor); // keeps capacity
bool vektor_clear(Vektor* vektor); // deallocates

void* vektor_add(Vektor* vektor, const void* data);
void* vektor_insert(Vektor* vektor, vektor_size_t location, void* data);

vektor_size_t vektor_size(Vektor* vektor);
void vektor_set_size(Vektor* vektor, vektor_size_t new_size);

void* vektor_get(Vektor* vektor, vektor_size_t index);
void* vektor_pop(Vektor* vektor);
void* vektor_last(Vektor* vektor);
bool vektor_remove(Vektor* vektor, vektor_size_t index);

#ifdef MARROW_VEKTOR_IMPLEMENTATION
#undef MARROW_VEKTOR_IMPLEMENTATION

Vektor* vektor_create(vektor_size_t initial_capacity, vektor_size_t element_size)
{
    Vektor* vektor = malloc(sizeof(Vektor));
    vektor_init(vektor, initial_capacity, element_size);
    return vektor;
}

bool vektor_init(Vektor* vektor, vektor_size_t initial_capacity, vektor_size_t element_size)
{
    *vektor = (Vektor){
      .data = initial_capacity ? malloc(initial_capacity * element_size) : nullptr,
      .size = 0,
      .capacity = initial_capacity,
      .element_size = element_size
    };
    return true;
}

bool vektor_destroy(Vektor** vektor_ptr)
{
    if(!vektor_ptr)
        return false;

    Vektor* vektor = *vektor_ptr;
    vektor_clear(vektor);
    free(vektor);
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
    free(vektor->data);
    vektor->size = 0;
    vektor->capacity = 0;
    vektor->data = nullptr;

    return true;
}

void internal_vektor_grow(Vektor* vektor)
{
    vektor->capacity = vektor->capacity * 2 + 1;
    if (!vektor->data){
        vektor->data = malloc(vektor->capacity * vektor->element_size);
        return;
    }
    vektor->data = realloc(vektor->data, vektor->capacity * vektor->element_size);
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

void* vektor_insert(Vektor* vektor, vektor_size_t location, void* data)
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
