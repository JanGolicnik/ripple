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

#ifndef marrow_unused
#define marrow_unused (void)
#endif // marrow_unused

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

#ifndef struct
#define struct(name) typedef struct name name; struct name
#endif // struct

struct(s8) {
    u8* ptr;
    usize size;
};

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

#ifndef marrow_format
thread_local s8 _format_buf;
#define marrow_format(f, allocator, ...)\
(\
    _format_buf.size = print(0, 0, f, __VA_ARGS__),\
    _format_buf.ptr = allocator_alloc((Allocator*)allocator, _format_buf.size + 1, 1),\
    (void)print((char*)_format_buf.ptr, _format_buf.size, f, __VA_ARGS__),\
    _format_buf\
)
#endif // marrow_format

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

u32 str_len(const char* str)
{
    const char* iter = str;
    while (*iter) iter++;
    return iter - str;
}

u32 s8_cmp(s8 a, s8 b)
{
    if (a.size != b.size) return a.size - b.size;
    return buf_cmp(a.ptr, b.ptr, a.size);
}


#ifndef LINE_UNIQUE_VAR
#define LINE_UNIQUE_VAR_CONCAT(a, b) a##b
#define LINE_UNIQUE_VAR_PASS(a, b) LINE_UNIQUE_VAR_CONCAT(a, b)
#define LINE_UNIQUE_VAR(var) LINE_UNIQUE_VAR_PASS(var, __LINE__)
#define LINE_UNIQUE_I LINE_UNIQUE_VAR(i)
#endif //LINE_UNIQUE_VAR


#ifndef for_each_n
#define for_each_n(el, ptr, n) u32 LINE_UNIQUE_I = 0; for(typeof(*(ptr))* el = &ptr[LINE_UNIQUE_I]; LINE_UNIQUE_I < n; LINE_UNIQUE_I++, el = &ptr[LINE_UNIQUE_I])
#endif // for_each_n

#ifndef for_each
#define for_each(el, ptr) u32 LINE_UNIQUE_I = 0; for(typeof(*(ptr))* el = &ptr[LINE_UNIQUE_I]; LINE_UNIQUE_I < array_len(ptr); LINE_UNIQUE_I++, el = &ptr[LINE_UNIQUE_I])
#endif // for_each

#ifndef for_each_i
#define for_each_i(el, ptr, i) for(u32 i = 0, LINE_UNIQUE_I = 1; i < array_len(ptr); i++, LINE_UNIQUE_I = 1) for(typeof(*(ptr)) *el = &ptr[i]; LINE_UNIQUE_I ; LINE_UNIQUE_I = 0)
#endif // for_each_i

u64 s8_hash(s8 buf)
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

// insertion sort
#define sort_indices(out, array, n, cmp) do {\
    for (u32 i = 0; i < n; i++) {\
        typeof(*array)* a = &array[i];\
        u32 index = 0;\
        while(index < i) {\
            typeof(*array)* b = &array[out[index]];\
            if (cmp) break;\
            index++;\
        }\
        for (u32 j = i; j > index; j--) {\
            out[j] = out[j - 1];\
        }\
        out[index] = i;\
    }\
} while(false)

#define mergesort_indices(out, array, n, cmp) do {\
    u32 aux[(n)];\
    for (u32 i = 0; i < (n); i++) out[i] = i;\
    for (u32 width = 1; width < (n); width *= 2) {\
        for (u32 low = 0; low < (n); low += 2 * width) {\
            u32 mid = ((low + width) < (n)) ? low + width : (n);\
            u32 high = (low + 2 * width < (n)) ? low + 2 * width : (n);\
            u32 i = low, j = mid, k = low;\
            while (i < mid && j < high) {\
                typeof(*array)* a = &array[out[i]];\
                typeof(*array)* b = &array[out[j]];\
                aux[k++] = (!(cmp)) ? out[i++] : out[j++];\
            }\
            while (i < mid) {\
                aux[k++] = out[i++];\
            }\
            while (j < high) {\
                aux[k++] = out[j++];\
            }\
            for (k = low; k < high; k++) {\
                out[k] = aux[k];\
            }\
        }\
    }\
} while(false)

u32 value_to_rgb(f32 value)
{
    value = clamp(value, 0.0f, 1.0f);
    return ((u8)(value * 0xff) << 16) | ((u8)(value * 0xff) << 8) | (u8)(value * 0xff);
}

u32 to_byte(f32 x){
    // x in [0,1], round to nearest and clamp
    int v = (int)lrintf(x * 255.0f);
    if (v < 0) v = 0; else if (v > 255) v = 255;
    return (u32)v;
}

struct(HSV) {
    f32 hue;
    f32 saturation;
    f32 value;
};

u32 hsv_to_rgb(HSV hsv)
{
    hsv.value = clamp(hsv.value, 0.0f, 1.0f);
    hsv.saturation = clamp(hsv.saturation, 0.0f, 1.0f);
    if (!isfinite(hsv.hue)) hsv.hue = 0; // hue irrelevant when S==0 or V==0; pick any
    hsv.hue = wrap_float(hsv.hue, 360.0f);

    f32 c = hsv.value * hsv.saturation;
    f32 h6 = hsv.hue / 60.0f;         // 0..6
    f32 x = c * (1.0f - fabsf(fmodf(h6, 2.0f) - 1.0f));
    f32 m = hsv.value - c;

    f32 r=0, g=0, b=0;
    if      (h6 < 1) { r=c; g=x; b=0; }
    else if (h6 < 2) { r=x; g=c; b=0; }
    else if (h6 < 3) { r=0; g=c; b=x; }
    else if (h6 < 4) { r=0; g=x; b=c; }
    else if (h6 < 5) { r=x; g=0; b=c; }
    else             { r=c; g=0; b=x; }

    u32 R = to_byte(r + m);
    u32 G = to_byte(g + m);
    u32 B = to_byte(b + m);
    return (R << 16) | (G << 8) | B;   // 0xRRGGBB
}

HSV rgb_to_hsv(u32 color) {
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

    return (HSV){wrap_float(H, 360.0f), clamp(S, 0.0f, 1.0f), clamp(V, 0.0f, 1.0f)};
}

#endif // MARROW_H
