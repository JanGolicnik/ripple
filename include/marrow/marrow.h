#ifndef MARROW_H
#define MARROW_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef MARROW_NO_PRINTCCY
#include <printccy/printccy.h>
#endif // MARROW_NO_PRINTCCY

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

#ifndef LINE_UNIQUE_I
#define LINE_UNIQUE_I_CONCAT(a, b) a##b
#define LINE_UNIQUE_I_PASS(a, b) LINE_UNIQUE_I_CONCAT(a, b)
#define LINE_UNIQUE_I LINE_UNIQUE_I_PASS(i, __LINE__)
#endif //LINE_UNIQUE_I

#ifndef for_each
#define for_each(el, ptr, n) u32 LINE_UNIQUE_I = 0; for(typeof(*(ptr))* el = &ptr[LINE_UNIQUE_I]; LINE_UNIQUE_I < n; LINE_UNIQUE_I++, el = &ptr[LINE_UNIQUE_I])
#endif // for_each

#ifndef for_each_i
#define for_each_i(el, ptr, n, i) for(u32 i = 0, LINE_UNIQUE_I = 1; i < n; i++, LINE_UNIQUE_I = 1) for(typeof(*(ptr)) *el = &ptr[i]; LINE_UNIQUE_I ; LINE_UNIQUE_I = 0)
#endif // for_each_i

#define thread_local _Thread_local

#endif // MARROW_H
