#ifndef MARROW_MAPA_H
#define MARROW_MAPA_H

#include "marrow.h"

#include <string.h>

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


mapa_hash_t mapa_hash_djb2(void const* v_key, mapa_size_t key_size)
{
  // http://www.cse.yorku.ca/~oz/hash.html
  u8 const* key = v_key;
  mapa_hash_t hash = MAPA_INITIAL_SEED;
  for(u32 i = 0; i < key_size; i++)
    hash = ((hash << 5) + hash) + *(key++); /* hash * 33 + c */
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

#endif // MARROW_MAPA_H
