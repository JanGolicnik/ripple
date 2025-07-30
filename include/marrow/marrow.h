#ifndef MARROW_H
#define MARROW_H

#include <stdint.h>

#ifndef MARROW_NO_PRINTCCY
#include <printccy/printccy.h>
#endif // MARROW_NO_PRINTCCY

typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;

typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;

typedef uintmax_t     usize;

typedef float         f32;
typedef double        f64;

typedef _Bool         bool;

const u8  U8_MAX  = ~(u8) 0;
const u16 U16_MAX = ~(u16)0;
const u32 U32_MAX = ~(u32)0;
const u64 U64_MAX = ~(u64)0;

const i8  I8_MAX  =  (i8)  ((1 << 7) - 1);
const i16 I16_MAX =  (i16) ((1 << 15) - 1);
const i32 I32_MAX =  (i32) ((1u << 31) - 1);
const i64 I64_MAX =  (i64) ((1ull << 63) - 1);

#ifndef true
#define true 1
#endif // true

#ifndef false
#define false 0
#endif // false

#ifndef nullptr
#define nullptr (void*)0
#endif // nullptr

#ifndef NULL
#define NULL {}
#endif // NULL

#define BIT(n) (1ULL << (n))
#define BIT_IS_SET(val,n)  (((val) >> (n)) & 1)
#define BIT_SET(val, n)   ((val) |= BIT(n))
#define BIT_CLEAR(val, n) ((val) &= ~BIT(n))
#define BIT_TOGGLE(val,n) ((val) ^= BIT(n))

#ifndef loop
#define loop while(true)
#endif // loop

#ifndef unused
#define unused (void)
#endif //unused

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // max

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif // min

#ifndef clamp
#define clamp(val, low, high) min(max(val, low), high)
#endif // clamp

#ifndef is_between
#define is_between(val, low, high) ((val) > (low) && (val) < (high))
#endif // is_between

#ifndef is_between_inclusive
#define is_between_inclusive(val, low, high) ((val) >= (low) && (val) <= (high))
#endif // is_between_inclusive

#ifndef array_len
#define array_len(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif // array_len

#ifndef push_stream
#define push_stream(stream) fflush(stream)
#endif // push_stream

#ifndef debug_color
#define debug_color "\x1b[92m"
#endif // debug_color

#ifndef error_color
#define error_color "\x1b[91m"
#endif // error_color

#ifndef text_color
#define text_color "\x1b[0m\x1b[90m"
#endif // text_color

#ifdef MARROW_NO_PRINTCCY

#ifndef debug
#define debug(format, ...) do { fprintf(stderr, debug_color "[DEBUG]" text_color " %s on line %d: \x1b[0m " format "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); } while(0)
#endif // debug

#ifndef error
#define error(format, ...) do { fprintf(stderr, error_color "[ERROR]" text_color " %s on line %d: \x1b[0m " format "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); } while(0)
#endif // error

#ifndef abort
#define abort(format, ...) do { fprintf(stderr, error_color "[ABORT]" text_color " %s on line %d: \x1b[0m " format "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); exit(1); } while(0)
#endif // abort

#else // MARROW_NO_PRINTCCY

#ifndef debug
#define debug(format, ...) do { printfb(stderr, debug_color "[DEBUG]" text_color " {} on line {}: \x1b[0m" format "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // debug

#ifndef error
#define error(format, ...) do { printfb(stderr, error_color "[ERROR]" text_color " {} on line {}: \x1b[0m" format "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // error

#ifndef abort
#define abort(format, ...) do { printfb(stderr, error_color "[ABORT]" text_color " {} on line {}: \x1b[0m" format "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); exit(1); } while(0)
#endif // abort

#endif // MARROW_NO_PRINTCCY

u32 str_len(const char* str)
{
    const char* iter = str;
    while (*iter) iter++;
    return iter - str;
}

void buf_copy(void* dst, const void* source, usize len)
{
    while(len--) ((u8*)dst)[len] = ((u8*)source)[len];
}

i32 buf_cmp(const void* a, const void* b, usize len)
{
    for (usize i = 0; i < len; i++) {
        if (((u8*)a)[i] != ((u8*)b)[i])
            return (i32)((u8*)a)[i] - (i32)((u8*)b)[i];
    }
    return 0;
}

void buf_set(void* dst, u8 value, usize len)
{
    while (len--) ((u8*)dst)[len] = value;
}

#ifndef LINE_UNIQUE_VAR
#define LINE_UNIQUE_VAR_CONCAT(a, b) a##b
#define LINE_UNIQUE_VAR_PASS(a, b) LINE_UNIQUE_VAR_CONCAT(a, b)
#define LINE_UNIQUE_VAR(var) LINE_UNIQUE_VAR_PASS(var, __LINE__)
#define LINE_UNIQUE_I LINE_UNIQUE_VAR(i)
#endif //LINE_UNIQUE_VAR

#ifndef for_each
#define for_each(el, ptr, n) u32 LINE_UNIQUE_I = 0; for(typeof(*(ptr))* el = &ptr[LINE_UNIQUE_I]; LINE_UNIQUE_I < n; LINE_UNIQUE_I++, el = &ptr[LINE_UNIQUE_I])
#endif // for_each

#ifndef for_each_i
#define for_each_i(el, ptr, n, i) for(u32 i = 0, LINE_UNIQUE_I = 1; i < n; i++, LINE_UNIQUE_I = 1) for(typeof(*(ptr)) *el = &ptr[i]; LINE_UNIQUE_I ; LINE_UNIQUE_I = 0)
#endif // for_each_i

#define thread_local _Thread_local

u64 hash_buf(const u8* buf, size_t buf_size)
{
    u64 hash = 0xcbf29ce484222325ULL;
    while (buf_size--) {
        hash ^= (u8)(*buf++);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// expects a null terminated string
u64 hash_str(const char *str)
{
    u64 hash = 0xcbf29ce484222325ULL;
    while (*str) {
        hash ^= (u8)(*str++);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

u64 hash_u64(u64 val)
{
    val ^= val >> 33;
    val *= 0xff51afd7ed558ccdULL;
    val ^= val >> 33;
    val *= 0xc4ceb9fe1a85ec53ULL;
    val ^= val >> 33;
    return val;
}

u64 hash_combine(u64 a, u64 b)
{
    u64 x = a ^ b;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

#define LINE_UNIQUE_HASH hash_combine(hash_str(__FILE__), hash_u64(__LINE__))

#endif // MARROW_H
