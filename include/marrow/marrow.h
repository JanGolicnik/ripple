#ifndef MARROW_H
#define MARROW_H

#include <stdint.h>
#include <math.h>

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

#ifndef alignof
#define alignof _Alignof
#endif // alignof

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif // max

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif // min

#ifndef clamp
#define clamp(val, low, high) min(max(val, low), high)
#endif // clamp

static f32 wrap_float(f32 val, f32 max) {
    val = fmodf(val, max);
    if (val < 0.f) val += max;
    return val;
}

#ifndef is_between
#define is_between(val, low, high) ((val) > (low) && (val) < (high))
#endif // is_between

#ifndef is_between_inclusive
#define is_between_inclusive(val, low, high) ((val) >= (low) && (val) <= (high))
#endif // is_between_inclusive

#ifndef array_len
#define array_len(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif // array_len

typedef struct {
    u8* ptr;
    usize size;
} s8;

#define S8(str) (s8){ .ptr = (u8*)str, .size = sizeof(str) }

#ifndef push_stream
#define push_stream(stream) fflush(stream)
#endif // push_stream

#ifndef thread_local
#define thread_local _Thread_local
#endif // thread_local

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
#define debug(f, ...) do { fprintf(stderr, debug_color "[DEBUG]" text_color " %s on line %d: \x1b[0m " f "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); } while(0)
#endif // debug

#ifndef error
#define error(f, ...) do { fprintf(stderr, error_color "[ERROR]" text_color " %s on line %d: \x1b[0m " f "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); } while(0)
#endif // error

#ifndef abort
#define abort(f, ...) do { fprintf(stderr, error_color "[ABORT]" text_color " %s on line %d: \x1b[0m " f "\n", __FILE__, __LINE__, __VA_ARGS__); push_stream(stderr); exit(1); } while(0)
#endif // abort

#else // MARROW_NO_PRINTCCY

#ifndef format
thread_local s8 _format_buf;
#define format(f, allocator, ...)\
(\
    _format_buf.size = print(0, 0, f, __VA_ARGS__),\
    _format_buf.ptr = allocator_alloc((Allocator*)allocator, _format_buf.size + 1, 1),\
    (void)print((char*)_format_buf.ptr, _format_buf.size, f, __VA_ARGS__),\
    _format_buf\
)
#endif // format

#ifndef debug
#define debug(f, ...) do { printfb(stderr, debug_color "[DEBUG]" text_color " {} on line {}: \x1b[0m" f "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // debug

#ifndef error
#define error(f, ...) do { printfb(stderr, error_color "[ERROR]" text_color " {} on line {}: \x1b[0m" f "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); } while(0)
#endif // error

#ifndef abort
#define abort(f, ...) do { printfb(stderr, error_color "[ABORT]" text_color " {} on line {}: \x1b[0m" f "\n", __FILE__, __LINE__ ,##__VA_ARGS__); push_stream(stderr); exit(1); } while(0)
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

u64 hash_buf(s8 buf)
{
    u64 hash = 0xcbf29ce484222325ULL;
    while (buf.size--) {
        hash ^= (u8)(*buf.ptr++);
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

#define INV_SQRT_3 0.5773502691896258f  // 1/sqrt(3)

#define array_get_sorted_indices(out, array, n, cmp) do {\
    for (u32 i = 0; i < n; i++) {\
        typeof(*array)* a = &array[i];\
        u32 index = 0;\
        while(index < i)\
        {\
            typeof(*array)* b = &array[out[index]];\
            if (cmp) break;\
            index++;\
        }\
        for (u32 j = i; j > index; j--)\
        {\
            out[j] = out[j - 1];\
        }\
        out[index] = i;\
    }\
} while( 0 )

u32 hsv_to_rgb(f32 hue, f32 saturation, f32 value) {
    f32 h = wrap_float(hue, 360.0f);
    f32 s = clamp(saturation, 0.0f, 1.0f);
    f32 v = clamp(value, 0.0f, 1.0f);

    f32 r, g, b;

    if (s == 0.f) {
        r = g = b = v;
    } else {
        f32 c = v * s;
        f32 hp = h / 60.f;
        f32 x  = c * (1.f - fabsf(fmodf(hp, 2.f) - 1.f));
        f32 r1=0.f, g1=0.f, b1=0.f;

        if      (0.f <= hp && hp < 1.f) { r1 = c; g1 = x; b1 = 0.f; }
        else if (1.f <= hp && hp < 2.f) { r1 = x; g1 = c; b1 = 0.f; }
        else if (2.f <= hp && hp < 3.f) { r1 = 0.f; g1 = c; b1 = x; }
        else if (3.f <= hp && hp < 4.f) { r1 = 0.f; g1 = x; b1 = c; }
        else if (4.f <= hp && hp < 5.f) { r1 = x; g1 = 0.f; b1 = c; }
        else                             { r1 = c; g1 = 0.f; b1 = x; }

        f32 m = v - c;
        r = r1 + m; g = g1 + m; b = b1 + m;
    }

    u32 R = (u32)lroundf(fmaxf(0.f, fminf(r, 1.f)) * 255.f);
    u32 G = (u32)lroundf(fmaxf(0.f, fminf(g, 1.f)) * 255.f);
    u32 B = (u32)lroundf(fmaxf(0.f, fminf(b, 1.f)) * 255.f);

    return (R << 16) | (G << 8) | B;
}

void rgb_to_hsv(u32 color, f32* hue, f32* saturation, f32* value) {
    f32 r = (f32)((color >> 16) & 0xFF) / 255.f;
    f32 g = (f32)((color >>  8) & 0xFF) / 255.f;
    f32 b = (f32)( color        & 0xFF) / 255.f;

    f32 maxv = fmaxf(r, fmaxf(g, b));
    f32 minv = fminf(r, fminf(g, b));
    f32 delta = maxv - minv;

    f32 V = maxv;

    f32 S = (maxv <= 0.f) ? 0.f : (delta / maxv);

    f32 H;
    if (delta == 0.f) {
        H = 0.f;
    } else if (maxv == r) {
        H = 60.f * fmodf(((g - b) / delta), 6.f);
        if (H < 0.f) H += 360.f;
    } else if (maxv == g) {
        H = 60.f * (((b - r) / delta) + 2.f);
    } else {
        H = 60.f * (((r - g) / delta) + 4.f);
    }

    if (hue)        *hue = wrap_float(H, 360.0f);
    if (saturation) *saturation = clamp(S, 0.0f, 1.0f);
    if (value)      *value = clamp(V, 0.0f, 1.0f);
}

#endif // MARROW_H
