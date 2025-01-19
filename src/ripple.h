#ifndef RIPPLE_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#endif

#include <marrow/marrow.h>

typedef enum {
    SVT_INVALID = 0,
    SVT_PERCENT,
    SVT_PIXELS
} RippleSizingValueType;

typedef struct {
    RippleSizingValueType _type;
    union{
        f32 percent;
        u32 pixels;
    } _value;
} RippleSizingValue;

#define DEPTH(value) (SizingValue) {\
    ._type = SVT_PERCENT,\
    ._value.percent = value\
}

#define BUBBLES(value) (SizingValue) {\
    ._type = SVT_PIXELS,\
    ._value.pixels = value\
}

typedef u32 Color;

typedef struct {
    bool centered;
    RippleSizingValue width;
    RippleSizingValue height;
    RippleSizingValue min_width;
    RippleSizingValue min_height;
} RippleElementLayout;

#define WAVE(...) (RippleElementLayout) { __VA_ARGS__ }

typedef enum {
    RET_INVALID = 0,
    RET_BUTTON,
    RET_TEXT
} RippleElementType;

typedef struct{
    RippleElementType _type;
    Color color;
} RippleButtonConfig;

#define FISH(...) (RippleButtonConfig) {\
        ._type = RET_BUTTON,\
        __VA_ARGS__\
    }

typedef struct{
    RippleElementType _type;
    const char* content;
    RippleSizingValue font_size;
} RippleTextConfig;

#define CURRENT(...) (RippleTextConfig) {__VA_ARGS__}
#define PARENT
#define CHILD
#define GROW(...) {}
#define FIXED(...) {}

#define HOVERED() true

typedef struct {
    u32 width;
    u32 height;
} RippleWindowConfig;

void Ripple_start_window(const char* name, RippleWindowConfig config);
void Ripple_finish_window(const char* name);

void Ripple_start_element(const char* name, RippleElementLayout layout);
void Ripple_finish_element(const char* name, void* element);

#define RIPPLE_UNIQUE_I i##__FILE__##__LINE__

#define LAKE(name, ...) \
    for (u8 RIPPLE_UNIQUE_I = (Ripple_start_window(name, (RippleWindowConfig) { __VA_ARGS__ }), 0); \
    RIPPLE_UNIQUE_I < 1; \
    Ripple_finish_window(name), RIPPLE_UNIQUE_I++)

#define RIPPLE(name, layout, ...) \
    for (u8 RIPPLE_UNIQUE_I = (Ripple_start_element(name, layout), 0); \
    RIPPLE_UNIQUE_I < 1; \
    Ripple_finish_element(name, (void*)&__VA_ARGS__), RIPPLE_UNIQUE_I++)

typedef struct {
    bool initialized;
    u32 width;
    u32 height;
} Window;

Window current_window;

void Ripple_start_window(const char* name, RippleWindowConfig config)
{
    current_window = (Window) {
            .initialized = true,
            .width = config.width,
            .height = config.height
    };
}

void Ripple_finish_window(const char* name)
{
    current_window = (Window) {};
}

void Ripple_start_element(const char* name, RippleElementLayout layout)
{

}

void Ripple_finish_element(const char* name, void* element) {}
