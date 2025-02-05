#ifndef MARROW_H
#define MARROW_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t        u8;
typedef uint16_t       u16;
typedef uint32_t       u32;
typedef uint64_t       u64;

typedef int8_t         i8;
typedef int16_t        i16;
typedef int32_t        i32;
typedef int32_t        i64;

typedef float          f32;
typedef double         f64;

typedef _Bool          bool;

const u8  U8_MAX  = (u8) ~0;
const u16 U16_MAX = (u16)~0;
const u32 U32_MAX = (u32)~0;
const u64 U64_MAX = (u64)~0;

const i8  I8_MAX  = (i8) (U8_MAX  >> 1);
const i16 I16_MAX = (i16)(U16_MAX >> 1);
const i32 I32_MAX = (i32)(U32_MAX >> 1);
const i64 I64_MAX = (i64)(U64_MAX >> 1);

#ifndef true
#define true 1
#endif // true

#ifndef false
#define false 0
#endif // false

#ifndef nullptr
#define nullptr 0
#endif // nullptr

#ifndef NULL
#define NULL {}
#endif // NULL

#ifndef loop
#define loop while(true)
#endif // loop

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // max

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif // min

#ifndef clamp
#define clamp(val, low, high) min(max(val, low), high)
#endif // clamp

#ifndef push_stream
#define push_stream(stream) fflush(stream)
#endif // push_stream

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    push_stream(stdout);
}

void println(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    print("\n");
    va_end(args);
    push_stream(stdout);
}

char* format(const char* format, ...) {
    va_list args;
    va_start(args, format);

    i32 len = vsnprintf(nullptr, 0, format, args);
    if (len <= 0)
        return nullptr;

    char* result = malloc(len + 1);

    va_start(args, format);
    vsnprintf(result, len + 1, format, args);

    va_end(args);

    return result;
}

#ifndef debug_color
#define debug_color "\x1b[92m"
#endif // debug_color

#ifndef error_color
#define error_color "\x1b[91m"
#endif // error_color

#ifndef text_color
#define text_color "\x1b[0m\x1b[90m"
#endif // text_color

#ifndef debug
#define debug(format, ...) do { fprintf(stderr, debug_color "[DEBUG]" text_color " %s on line %d:\x1b[0m " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // debug

#ifndef error
#define error(format, ...) do { fprintf(stderr, error_color "[ERROR]" text_color " %s on line %d:\x1b[0m " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // error

#ifndef abort
#define abort(format, ...) do { fprintf(stderr, error_color "[ABORT]" text_color " %s on line %d:\x1b[0m " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); push_stream(stderr); exit(1); } while(0)
#endif // abort

#ifndef LINE_UNIQUE_I
#define LINE_UNIQUE_I_CONCAT(a, b) a##b
#define LINE_UNIQUE_I_PASS(a, b) LINE_UNIQUE_I_CONCAT(a, b)
#define LINE_UNIQUE_I LINE_UNIQUE_I_PASS(i, __LINE__)
#endif //LINE_UNIQUE_I

#ifndef for_each
#define for_each(el, ptr, n) u32 LINE_UNIQUE_I = 0; for(typeof(*(ptr)) el = ptr[LINE_UNIQUE_I]; LINE_UNIQUE_I < n; LINE_UNIQUE_I++, el = ptr[LINE_UNIQUE_I])
#endif // for_each

#ifndef for_each_i
#define for_each_i(el, ptr, n, i) for(u32 i = 0, LINE_UNIQUE_I = 1; i < n; i++, LINE_UNIQUE_I = 1) for(typeof(*(ptr)) el = ptr[i]; LINE_UNIQUE_I ; LINE_UNIQUE_I = 0)
#endif // for_each_i

// VEKTOR

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


// MAPA

typedef u64 mapa_size_t;
typedef u64 mapa_hash_t;
typedef mapa_hash_t (*mapa_hash_func)(void const*, mapa_size_t);
typedef u8 (*mapa_cmp_func)(void const*, mapa_size_t, void const*, mapa_size_t);

#ifndef MAPA_INITIAL_CAPACITY
#define MAPA_INITIAL_CAPACITY 1
#endif // MAPA_INITIAL_CAPACITY

#ifndef MAPA_INITIAL_SEED
#define MAPA_INITIAL_SEED 0x9747b28c
#endif // MAPA_INITIAl_SEED

typedef struct {
    void* data;
    mapa_size_t size;
} MapaItem;

#ifdef MARROW_MAPA_IMPLEMENTATION

typedef struct{
  void* key;
  mapa_size_t key_size;
  MapaItem item;
} MapaEntry;

typedef struct
{
  MapaEntry* entries;
  mapa_size_t capacity;
  mapa_size_t size;

  mapa_hash_func hash_func;
  mapa_cmp_func cmp_func;
} Mapa;

#else

typedef void Mapa;

#endif

mapa_hash_t mapa_hash_djb2(void const* key, mapa_size_t key_size);
mapa_hash_t mapa_hash_fnv(void const* key, mapa_size_t key_size);
mapa_hash_t mapa_hash_MurmurOAAT_32(void const* key, mapa_size_t key_size);
u8 mapa_cmp_bytes(void const* a, mapa_size_t a_size, void const* b, mapa_size_t b_size);

Mapa* mapa_create(mapa_hash_func hash_func, mapa_cmp_func cmp_func);
bool mapa_destroy(Mapa* mapa);

bool mapa_insert(Mapa* mapa, void const* key, mapa_size_t key_size, void* data, mapa_size_t data_size);
bool mapa_insert_str(Mapa* mapa, char const* key, char* data); //expects null terminated string

MapaItem* mapa_get(Mapa* mapa, void const* key, mapa_size_t key_size);
MapaItem* mapa_get_str(Mapa* mapa, void const* key); // expects null terminated string
bool mapa_remove(Mapa* mapa, void const* key, mapa_size_t key_size);
bool mapa_remove_str(Mapa* mapa, void const* key); // expect null terminated string

#ifdef MARROW_MAPA_IMPLEMENTATION
#undef MARROW_MAPA_IMPLEMENTATION

MapaItem* mapa_get(Mapa* mapa, void const* key, mapa_size_t key_len);

void internal_mapa_grow(Mapa* mapa, mapa_size_t new_capacity)
{
  MapaEntry *new_entries = calloc(new_capacity, sizeof(MapaEntry));

  for (mapa_size_t i = 0; i < mapa->capacity; i++)
  {
    MapaEntry *entry = &mapa->entries[i];
    if (entry->key == nullptr)
      continue;

    mapa_size_t index = mapa->hash_func(entry->key, entry->key_size) % new_capacity;
    while(new_entries[index].key != nullptr)
      index = (index + 1) % new_capacity;

    new_entries[index] = *entry;
  }

  free(mapa->entries);
  mapa->entries = new_entries;
  mapa->capacity = new_capacity;
}

bool mapa_insert(Mapa* mapa, void const* key, mapa_size_t key_size, void* data, mapa_size_t data_size)
{
  MapaItem new_item = (MapaItem){ .data = malloc(data_size), .size = data_size};
  memcpy(new_item.data, data, data_size);

  MapaItem *item = mapa_get(mapa, key, key_size);
  if(item)
  {
    free(item->data);
    *item = new_item;
    return true;
  }

  if (mapa->size >= mapa->capacity * 0.55)
    internal_mapa_grow(mapa, mapa->capacity == 0 ? 1 : mapa->capacity * 2);

  mapa_size_t index = mapa->hash_func(key, key_size) % mapa->capacity;
  while(true)
  {
    if (mapa->entries[index].key == nullptr)
    {
      mapa->size += 1; // only update the size if inserting a completely new element
      break;
    }

    MapaEntry* entry = &mapa->entries[index];
    if(mapa->cmp_func(entry->key, entry->key_size, key, key_size) == 0)
    {
      free(entry->item.data);
      free(entry->key);
      break;
    }

    index = (index + 1) % mapa->capacity;
  }

  MapaEntry entry = (MapaEntry){.key = malloc(key_size), .key_size = key_size, .item = new_item };
  memcpy(entry.key, key, key_size);

  mapa->entries[index] = entry;
  return true;
}

bool mapa_insert_str(Mapa* mapa, char const* key, char* data)
{
  return mapa_insert(mapa, key, strlen(key) + 1, data, strlen(data) + 1);
}

MapaItem* mapa_get(Mapa* mapa, void const* key, mapa_size_t key_size)
{
  if (mapa->size == 0)
    return nullptr;

  mapa_size_t index = mapa->hash_func(key, key_size) % mapa->capacity;
  for(u32 i = 0; i < mapa->capacity; i++)
  {
    MapaEntry *entry = &mapa->entries[index];
    if(mapa->cmp_func(entry->key, entry->key_size, key, key_size) == 0)
    {
      return &entry->item;
    }

    index = (index + 1) % mapa->size;
  }

  return nullptr;
}

MapaItem* mapa_get_str(Mapa* mapa, void const* key)
{
  return mapa_get(mapa, key, strlen(key) + 1);
}

bool mapa_remove(Mapa* mapa, void const* key, mapa_size_t key_size)
{
  mapa_size_t index = mapa->hash_func(key, key_size) % mapa->capacity;
  for (mapa_size_t i = 0; i < mapa->capacity; i++)
  {
    MapaEntry *entry = &mapa->entries[index];
    if (entry->key == nullptr)
      return true; // item doesnt exist

    if (mapa->cmp_func(entry->key, entry->key_size, key, key_size) != 0)
    {
      index = (index + 1) % mapa->capacity;
      continue;
    }

    free(entry->item.data);
    free(entry->key);
    memset(entry, 0, sizeof(*entry));
    mapa->size -= 1;

    // move remaining entries down
    for(mapa_size_t i = 0; i < mapa->capacity; i++)
    {
      mapa_size_t index = mapa->hash_func(key, key_size) % mapa->capacity;
      MapaEntry *entry = &mapa->entries[index];
      mapa_size_t next_index = (index + i + 1) % mapa->capacity;
      MapaEntry *next_entry = &mapa->entries[next_index];
      if(next_entry->key == nullptr ||
         next_index == mapa->hash_func(next_entry->key, next_entry->key_size))
        break; // if entry is where it should be or if slot is empty

      memcpy(entry, next_entry, sizeof(*entry));
      memset(next_entry, 0, sizeof(*entry));
    }

    return true;
  }

  return false;
}

bool mapa_remove_str(Mapa* mapa, void const* key)
{
  return mapa_remove(mapa, key, strlen(key) + 1);
}


Mapa* mapa_create(mapa_hash_func hash_func, mapa_cmp_func cmp_func)
{
  Mapa* mapa = (Mapa*)malloc(sizeof(Mapa));
  *mapa = (Mapa){.hash_func = hash_func, .cmp_func = cmp_func};
  internal_mapa_grow(mapa, MAPA_INITIAL_CAPACITY);
  return mapa;
}

bool mapa_destroy(Mapa* mapa)
{
  for (u32 i = 0; i < mapa->size; i++)
  {
    MapaEntry *entry = &mapa->entries[i];
    free(entry->key);
    free(entry->item.data);
  }

  free(mapa);
  return true;
}


mapa_hash_t mapa_hash_djb2(void const* key, mapa_size_t key_size)
{
  // http://www.cse.yorku.ca/~oz/hash.html
  mapa_hash_t hash = MAPA_INITIAL_SEED;
  for(u32 i = 0; i < key_size; i++)
    hash = ((hash << 5) + hash) + *((u8*)key++); /* hash * 33 + c */
  return hash;
}

mapa_hash_t mapa_hash_fnv(void const* key, mapa_size_t key_size)
{
  // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  mapa_hash_t hash = MAPA_INITIAL_SEED;
  hash ^= 2166136261UL;
  for(u32 i = 0; i < key_size; i++)
    hash = (hash ^ ((u8*)key)[i]) * 16777619;
  return hash;
}

u32 murmur_32_scramble(u32 k)
{
  k *= 0xcc932d51;
  k = (k << 15) | (k >> 17);
  k *= 0x1b873593;
  return k;
}

mapa_hash_t mapa_hash_MurmurOAAT_32(void const* key, mapa_size_t key_size)
{
  // https://en.wikipedia.org/wiki/MurmurHash
  mapa_hash_t hash = MAPA_INITIAL_SEED;

  for (mapa_size_t i = 0; i + 4 <= key_size; i += 4)
  {
    u32 group = ((u32*)key)[i];
    hash ^= murmur_32_scramble(group);
    hash = (hash << 13) | (hash >> 19);
    hash = hash * 5 + 0x36546b64;
  }

  u32 remaining = 0;
  u8 remaining_size = key_size % 4;
  for (u8 i = 0; i < remaining_size; i++)
    remaining = ((u8*)key)[key_size - remaining_size + i] | (remaining << 8);
  hash ^= murmur_32_scramble(remaining);

  hash ^= key_size;
  hash ^= hash >> 16;
  hash *= 0x85ebca6b;
  hash ^= hash >> 13;
  hash *= 0xc2b2ae35;
  hash ^= hash >> 16;
  return hash;
}

u8 mapa_cmp_bytes(void const* a, mapa_size_t a_size, void const* b, mapa_size_t b_size)
{
  return a_size == b_size ? memcmp(a, b, a_size) : -1;
}

#endif // MARROW_MAPA_IMPLEMENTATION

// MAPA

#endif // MARROW_H
