#include <marrow/marrow.h>

void Ripple_begin_element(const char* name, void* el) {}
void Ripple_end_element(const char* name) {}

#define RIPPLE(name, ...) \
    for (u8 i = (Ripple_begin_element(name, (void*)&__VA_ARGS__), 0); i < 1; Ripple_end_element(name), i++)

typedef enum {
    SVT_INVALID = 0,
    SVT_PERCENT,
    SVT_PIXELS
} SizingValueType;

typedef struct {
    SizingValueType _type;
    union{
        f32 percent;
        u32 pixels;
    } _value;
} SizingValue;

#define PERCENT(value) (SizingValue) {\
    ._type = SVT_PERCENT,\
    ._value.percent = value\
}\

typedef u32 Color;

typedef struct {
    bool centered;
    SizingValue width;
    SizingValue height;
    SizingValue min_width;
    SizingValue min_height;
} RippleElementLayout;

typedef enum {
    RET_INVALID = 0,
    RET_BUTTON,
    RET_TEXT
} RippleElementType;

typedef struct{
    RippleElementType _type;
    RippleElementLayout layout;
    Color color;
} RippleButtonConfig;

#define BUTTON(...) (RippleButtonConfig) {\
        ._type = RET_BUTTON,\
        __VA_ARGS__\
    }

#define TEXT(...) 0
#define PARENT
#define CHILD
#define GROW(...) {}
#define FIXED(...) {}

#define HOVERED() true
