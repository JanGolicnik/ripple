#ifndef MARROW_MAPA_H
#define MARROW_MAPA_H

#include "marrow.h"
#include "marrow/allocator.h"

#ifndef MAPA_INITIAL_CAPACITY
#define MAPA_INITIAL_CAPACITY 1
#endif // MAPA_INITIAL_CAPACITY

#ifndef MAPA_INITIAL_SEED
#define MAPA_INITIAL_SEED 0x9747b28c
#endif // MAPA_INITIAl_SEED

typedef u64 (*mapa_hash_func)(void const*, u64);
typedef u8 (*mapa_cmp_func)(void const*, void const*, u64);

#define MAPA(key_type, value_type) \
struct \
{ \
    struct { \
        /* DO NOT CHANGE ORDER */ \
        struct { \
            key_type key; \
            value_type value; \
        } v; \
        bool has_value; \
    }* entries; \
\
    u64 n_entries; \
    u64 size; \
\
    mapa_hash_func _hash_func; \
    mapa_cmp_func _cmp_func; \
    Allocator* _allocator; \
\
    /* TODO: add owning */ \
}

#define mapa_init(m, hash_func, cmp_func, allocator) \
do { \
    m._hash_func = hash_func; m._cmp_func = cmp_func; m._allocator = allocator; m.size = MAPA_INITIAL_CAPACITY; m.n_entries = 0;\
    m.entries = allocator_alloc(m._allocator, sizeof(m.entries[0]) * m.size); \
    buf_set(m.entries, 0, m.size * sizeof(*m.entries)); \
} while(0)

#define mapa_free(m) \
do { \
    /* TODO: if owning free keys as well */ \
    allocator_free(m._allocator, m.entries, m.size * sizeof(*m.entries)); \
    m.n_entries = 0; m.size = 0; m.entries = nullptr;\
} while(0)

typedef MAPA(u8, u8) _MAPA2;

// returns the index at which the element would be inserted if it existed
u64 _mapa_get_index(_MAPA2* mapa, void* key, u32 key_size, u32 v_size, u32 entry_size)
{
    if (mapa->size == 0) return -1;

    u8* entries = (u8*)mapa->entries;
    u64 index = mapa->_hash_func(key, key_size) % mapa->size;
    for (u32 i = 0; i < mapa->size; i++)
    {
        void* entry = entries + entry_size * index;
        bool has_value = *(bool*)((u8*)entry + v_size);
        if (has_value == false || mapa->_cmp_func(entry, key, key_size) == 0)
        {
            return index;
        }

        index = (index + 1) % mapa->size;
    }

    return -1;
}

#define mapa_get_index(m, key_ptr) (_mapa_get_index((void*)&m, key_ptr, sizeof(m.entries[0].v.key), sizeof(m.entries[0].v), sizeof(*m.entries)))

#define mapa_get_at_index(m, index) ((index <= m.size && m.entries[index].has_value) ? &m.entries[index].v.value : nullptr)

thread_local u64 _mapa_i = -1;
#define mapa_get(m, key) (_mapa_i = mapa_get_index(m, key), mapa_get_at_index(m, _mapa_i))

void _internal_mapa_grow(_MAPA2* mapa, u32 new_size, u32 key_size, u32 v_size, u32 entry_size)
{
    u32 alloc_size = new_size * entry_size;
    u8* new_entries = allocator_alloc(mapa->_allocator, alloc_size);
    buf_set(new_entries, 0, alloc_size);

    u8* entries = (u8*)mapa->entries;
    for (u64 i = 0; i < mapa->size; i++)
    {
        void* entry = entries + entry_size * i;
        bool has_value = *(bool*)((u8*)entry + v_size);
        if (has_value == false)
            continue;

        u64 index = mapa->_hash_func(entry, key_size) % new_size;
        while (*(bool*)(new_entries + entry_size * index + v_size))
            index = (index + 1) % new_size;

        buf_copy((void*)(new_entries + entry_size * index), entry, entry_size);
    }

    allocator_free(mapa->_allocator, mapa->entries, mapa->size * entry_size);
    mapa->entries = (void*)new_entries;
    mapa->size = new_size;
}

thread_local u64 _mapa_tmp_index = -1;
#define mapa_insert(m, key_ptr, _value) ( \
    m.n_entries >= m.size * 0.55 ? \
        _internal_mapa_grow((void*)&m, m.size * 2 + 1, sizeof((m).entries[0].v.key), sizeof((m).entries[0].v), sizeof((m).entries[0])) : \
            (void)0, \
    _mapa_tmp_index = mapa_get_index(m, key_ptr), \
    !m.entries[_mapa_tmp_index].has_value ? (void)m.n_entries++ : (void)0, \
    m.entries[_mapa_tmp_index].has_value = true, \
    m.entries[_mapa_tmp_index].v.key = *(key_ptr), \
    m.entries[_mapa_tmp_index].v.value = (_value), \
    &m.entries[_mapa_tmp_index].v.value \
)

#define mapa_remove_at_index(m, index) \
do { \
    if (index >= m.size) break; \
    m.entries[index].has_value = false; \
    m.n_entries--; \
    for (u32 i = 0; i < m.size; i++) \
    { \
        u32 next_index = (index + i + 1) % m.size; \
        if (m.entries[next_index].has_value == false || \
            next_index == (m._hash_func(&m.entries[next_index].v.key, sizeof((m.entries)->v.key)) % m.size)) \
            break; \
        m.entries[index] = m.entries[next_index]; \
        m.entries[next_index].has_value = false; \
    } \
} while(0)

#define mapa_remove(m, key) \
do { \
    mapa_remove_at_index(m, mapa_get_index(m, key)); \
} while (0)

u64 mapa_hash_djb2(void const* v_key, u64 key_size)
{
    // http://www.cse.yorku.ca/~oz/hash.html
    u8 const* key = v_key;
    u64 hash = MAPA_INITIAL_SEED;
    for(u32 i = 0; i < key_size; i++)
        hash = ((hash << 5) + hash) + *(key++); /* hash * 33 + c */
    return hash;
}

u64 mapa_hash_fnv(void const* key, u64 key_size)
{
    // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
    u64 hash = MAPA_INITIAL_SEED;
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

u64 mapa_hash_u64(void const* key, u64 key_size)
{
    return *(u64*)key;
}

u64 mapa_hash_u32(void const* key, u64 key_size)
{
    return *(u32*)key;
}

u64 mapa_hash_MurmurOAAT_32(void const* key, u64 key_size)
{
    // https://en.wikipedia.org/wiki/MurmurHash
    u64 hash = MAPA_INITIAL_SEED;

    for (u64 i = 0; i + 4 <= key_size; i += 4)
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

u8 mapa_cmp_bytes(void const* a, void const* b, u64 size)
{
    return buf_cmp(a, b, size);
}

#endif // MARROW_MAPA_H
