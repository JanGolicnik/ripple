#ifndef MARROW_H
#define MARROW_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

typedef float              f32;
typedef double             f64;

typedef _Bool              bool;

#ifndef true
#define true 1
#endif // true

#ifndef false
#define false 0
#endif // false

#ifndef NULL
#define NULL 0
#endif // NULL

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void println(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    print("\n");
    va_end(args);
}

char* format(const char* format, ...) {
    va_list args;
    va_start(args, format);

    i32 len = vsnprintf(NULL, 0, format, args);
    if (len <= 0)
        return NULL;

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
#define debug(format, ...) fprintf(stderr, debug_color "[DEBUG]" text_color " %s on line %d:\x1b[0m " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif // debug

#ifndef error
#define error(format, ...) fprintf(stderr, error_color "[ERROR]" text_color " %s on line %d:\x1b[0m " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif // error

#endif // MARROW_H
