#ifndef RIPPLE_NO_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_MAPA

#ifndef RIPPLE_NO_DEFINE_MARROW_VEKTOR
#define MARROW_VEKTOR_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_VEKTOR

#include <marrow/marrow.h>

typedef enum {
    SVT_NOT_SPECIFIED = 0,
    SVT_FIXED = 1,
    SVT_RELATIVE_CHILD = 2,
    SVT_RELATIVE_PARENT = 3
} RippleSizingValueType;

typedef struct {
    union{
        u32 _unsigned_value : 30;
        i32 _signed_value : 30;
    };
    RippleSizingValueType _type : 2;
} RippleSizingValue;

#define PARENT ._type = SVT_RELATIVE_PARENT
#define CHILD ._type = SVT_RELATIVE_CHILD
#define DEPTH(sizing_value, relation) { ._unsigned_value = (u32)(sizing_value * (f32)(2<<14)), relation }
#define BUBBLES(sizing_value) { ._signed_value = sizing_value, ._type = SVT_FIXED }

typedef u32 Color;

typedef struct {
    bool centered;
    RippleSizingValue x;
    RippleSizingValue y;
    RippleSizingValue width;
    RippleSizingValue height;
    RippleSizingValue min_width;
    RippleSizingValue min_height;
    RippleSizingValue max_width;
    RippleSizingValue max_height;
} RippleElementLayoutConfig;

static void print_element_layout(RippleElementLayoutConfig layout)
{
    #define PRINT_VALUE(name) layout.name._type, layout.name._type == SVT_FIXED ? (f32)layout.name._signed_value : (f32)layout.name._unsigned_value / (f32)(2<<14)
    println("ElementLayout { centered: %d,"
            " x: (%d, %.2f)"
            " y: (%d, %.2f)"
            " width: (%d, %.2f)"
            " height: (%d, %.2f)"
            " min_width: (%d, %.2f)"
            " min_height: (%d, %.2f)"
            " max_width: (%d, %.2f)"
            " min_width: (%d, %.2f)"
            "} ",
            layout.centered,
            PRINT_VALUE(x),
            PRINT_VALUE(y),
            PRINT_VALUE(width),
            PRINT_VALUE(height),
            PRINT_VALUE(min_width),
            PRINT_VALUE(min_height),
            PRINT_VALUE(max_width),
            PRINT_VALUE(max_height)
    );
    #undef PRINT_VALUE
}

#define WAVE(...) (RippleElementLayoutConfig) { __VA_ARGS__ }

typedef enum {
    RET_INVALID = 0,
    RET_BUTTON,
    RET_TEXT,
    RET_DEFAULT_END,
} RippleElementType;

typedef struct {
    RippleElementType _type;
    Color color;
} RippleButtonConfig;

#define FISH(...) (RippleButtonConfig) {\
        ._type = RET_BUTTON,\
        __VA_ARGS__\
    }

typedef struct {
    RippleElementType _type;
    const char* content;
    RippleSizingValue font_size;
} RippleTextConfig;

#define CURRENT(...) (RippleTextConfig) {._type = RET_TEXT, __VA_ARGS__}

#define HOVERED() true

typedef struct {
    u32 width;
    u32 height;
} RippleWindowConfig;

void Ripple_start_window(const char* name, RippleWindowConfig config);
void Ripple_finish_window(const char* name);

void Ripple_start_element(const char* name, RippleElementLayoutConfig layout);
void Ripple_finish_element(const char* name, void* element);

#define RIPPLE_UNIQUE_I i##__FILE__##__LINE__

#define LAKE(name, ...) \
    u8 RIPPLE_UNIQUE_I = 0; for (Ripple_start_window(name, (RippleWindowConfig) { __VA_ARGS__ }); RIPPLE_UNIQUE_I < 1; Ripple_finish_window(name), RIPPLE_UNIQUE_I++)

#define RIPPLE(name, layout, ...) \
    u8 RIPPLE_UNIQUE_I = 0; for (Ripple_start_element(name, layout); RIPPLE_UNIQUE_I < 1; Ripple_finish_element(name, (void*)&__VA_ARGS__), RIPPLE_UNIQUE_I++)

typedef struct {
    bool initialized;
    u32 width;
    u32 height;
} Window;

typedef struct {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
} RenderedLayout;

static void print_rendered_layout(RenderedLayout layout)
{
    println("RendredLayout { x: %d, y: %d, w: %d, h: %d }",
          layout.x,
          layout.y,
          layout.w,
          layout.h
        );
}

// aborts if value is of type SVT_NOT_SPECIFIED
i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value);

// returns or if value is SVT_NOT_SPECIFIED
#define CALCULATE_SIZING_OR(value, relative_member, or) \
    value._type == SVT_NOT_SPECIFIED ? or : \
    apply_relative_sizing(value, get_current_element_parent().relative_member, sum_up_current_element_children().relative_member)

#ifdef RIPPLE_IMPLEMENTATION
Vektor* rendered_layouts = nullptr;
Vektor* element_layouts = nullptr;
Vektor* parent_element_indices = nullptr;

Window current_window;

static RenderedLayout get_current_element_parent(void)
{
    u32 parent_element_indices_size = vektor_size(parent_element_indices);
    if (parent_element_indices_size <= 1)
    {
        abort("getting parent of root element");
    }

    u32 parent_element_index = *(u32*)vektor_get(parent_element_indices, parent_element_indices_size - 2);
    return *(RenderedLayout*)vektor_get(rendered_layouts, parent_element_index);
}

static RenderedLayout sum_up_current_element_children(void)
{
    RenderedLayout out_layout = {};
    u32 current_element_index = *(u32*)vektor_last(parent_element_indices);
    for (u32 i = current_element_index + 1; i < vektor_size(rendered_layouts); i++)
    {
        RenderedLayout child_layout = *(RenderedLayout*)vektor_get(rendered_layouts, i);
        out_layout.x = min(out_layout.x, child_layout.x);
        out_layout.y = min(out_layout.y, child_layout.y);
        out_layout.w = max(out_layout.w, child_layout.w);
        out_layout.h = max(out_layout.h, child_layout.h);
    }
    return out_layout;
}

i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value)
{
    if (value._type == SVT_NOT_SPECIFIED)
        abort("SVT_NOT_SPECIFIED");

    if (value._type == SVT_FIXED)
        return value._signed_value;

    i32 relative_value = value._type == SVT_RELATIVE_PARENT ? parent_value : children_value;
    f32 percentage_value = (f32)value._unsigned_value / (f32)(2<<14);
    return percentage_value * relative_value;
}

void Ripple_start_window(const char* name, RippleWindowConfig config)
{
    current_window = (Window) {
            .initialized = true,
            .width = config.width,
            .height = config.height
    };

    if (element_layouts) vektor_empty(element_layouts);
    else element_layouts = vektor_create(0, sizeof(RippleElementLayoutConfig));

    if (rendered_layouts) vektor_empty(rendered_layouts);
    else rendered_layouts = vektor_create(0, sizeof(RenderedLayout));

    if (parent_element_indices) vektor_empty(parent_element_indices);
    else parent_element_indices = vektor_create(0, sizeof(u32));

    // add root element data
    vektor_add(rendered_layouts, &(RenderedLayout) {
        .x = 0,
        .y = 0,
        .w = current_window.width,
        .h = current_window.height
    });
    vektor_add(element_layouts, &(RippleElementLayoutConfig) { });
    u32 root_index = 0;
    vektor_add(parent_element_indices, &root_index);

    debug("started window %s", name);
}

void Ripple_finish_window(const char* name)
{
    current_window = (Window) {};

    debug("finished window %s", name);
}

static RenderedLayout render_layout(RenderedLayout this_layout, RippleElementLayoutConfig layout)
{
    RenderedLayout parent_layout = get_current_element_parent();

    this_layout.w = CALCULATE_SIZING_OR(layout.width, w, parent_layout.w);
    i32 max_width = CALCULATE_SIZING_OR(layout.max_width, w, this_layout.w);
    i32 min_width = CALCULATE_SIZING_OR(layout.min_width, w, this_layout.w);
    this_layout.w = clamp(this_layout.w, min_width, max_width);

    i32 x = CALCULATE_SIZING_OR(layout.x, w, (parent_layout.w - this_layout.w) / 2);
    this_layout.x = parent_layout.x + x;

    this_layout.h = CALCULATE_SIZING_OR(layout.height, h, parent_layout.h);
    i32 max_height = CALCULATE_SIZING_OR(layout.max_height, h, this_layout.h);
    i32 min_height = CALCULATE_SIZING_OR(layout.min_height, h, this_layout.h);
    this_layout.h = clamp(this_layout.h, min_height, max_height);

    i32 y = CALCULATE_SIZING_OR(layout.y, h, (parent_layout.h - this_layout.h) / 2);
    this_layout.y = parent_layout.y + y;

    return this_layout;
}

static void print_all_rendered_layouts()
{
    for(u32 i = 0; i < vektor_size(rendered_layouts); i++)
        print_rendered_layout(*(RenderedLayout*)vektor_get(rendered_layouts, i));
}

void Ripple_start_element(const char* name, RippleElementLayoutConfig layout)
{
    // add this element as parent for further child elements
    u32 this_index = vektor_size(rendered_layouts);
    vektor_add(parent_element_indices, &this_index);

    RenderedLayout* rendered_layout = vektor_add(rendered_layouts, &(RenderedLayout){});
    vektor_add(element_layouts, &layout);

    // render out this element so children that are relative to parents still work
    // values for children will be rendered in the finish call
    *rendered_layout = render_layout(*rendered_layout, layout);

    // debug print
    debug("---------STARTED ELEMENT %s------------", name);
    print_element_layout(layout);
    print_rendered_layout(*rendered_layout);
}

void Ripple_finish_element(const char* name, void* element)
{
    u32 this_index = *(u32*)vektor_last(parent_element_indices);

    // render values that are relative to children
    RenderedLayout *rendered_layout = vektor_get(rendered_layouts, this_index);
    RippleElementLayoutConfig layout = *(RippleElementLayoutConfig*)vektor_get(element_layouts, this_index);
    *rendered_layout = render_layout(*rendered_layout, layout);

    // TODO: submit layout for rendering

    // delete all children and pop this index
    vektor_pop(parent_element_indices);
    vektor_set_size(rendered_layouts, this_index + 1);
    vektor_set_size(element_layouts, this_index + 1);

    debug("---------FINISHED ELEMENT %s------------", name);
    print_element_layout(layout);
    print_rendered_layout(*rendered_layout);
}

#endif // RIPPLE_IMPLEMENTATION
