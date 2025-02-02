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

typedef u32 Color;

typedef struct {
    bool fixed;
    RippleSizingValue x;
    RippleSizingValue y;
    RippleSizingValue width;
    RippleSizingValue height;
    RippleSizingValue min_width;
    RippleSizingValue min_height;
    RippleSizingValue max_width;
    RippleSizingValue max_height;
} RippleElementLayoutConfig;

typedef struct {
    RippleElementLayoutConfig layout;

    bool accept_input;

    // TODO: ptr to child ordering function

    // TODO: ptr to rendering function

    // TODO: ptr to misc data

} RippleElementConfig;

static void print_element_config(RippleElementConfig config)
{
    #define PRINT_VALUE(name) config.name._type, config.name._type == SVT_FIXED ? \
                (f64)config.name._signed_value : \
                (f64)config.name._unsigned_value / (f64)(2<<14)
    println("ElementConfig {"
                "ElementLayout {"
                " fixed: %d"
                " x: (%d, %.2f)"
                " y: (%d, %.2f)"
                " width: (%d, %.2f)"
                " height: (%d, %.2f)"
                " min_width: (%d, %.2f)"
                " min_height: (%d, %.2f)"
                " max_width: (%d, %.2f)"
                " min_width: (%d, %.2f)"
                "},"
            "} ",
            config.layout.fixed,
            PRINT_VALUE(layout.x),
            PRINT_VALUE(layout.y),
            PRINT_VALUE(layout.width),
            PRINT_VALUE(layout.height),
            PRINT_VALUE(layout.min_width),
            PRINT_VALUE(layout.min_height),
            PRINT_VALUE(layout.max_width),
            PRINT_VALUE(layout.max_height)
    );
    #undef PRINT_VALUE
}

typedef struct {
    i32 x;
    i32 y;
    i32 scroll;
    bool left_click;
    bool right_click;
    bool _valid;
} RippleCursorData;

typedef struct {
    RippleCursorData cursor_data;
    u32 width;
    u32 height;
} RippleWindowConfig;

typedef struct {
    RippleCursorData cursor_data;
    u32 width;
    u32 height;
    bool initialized;
} Window;

void Ripple_start_window(const char* name, RippleWindowConfig config);
void Ripple_finish_window(const char* name);

void Ripple_start_element(const char* name, RippleElementConfig config);
void Ripple_finish_element(const char* name);
void Ripple_submit_element(const char* name, void* user_data);

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

typedef struct {
    u32 clicked : 1;
    u32 hovered : 1;
    u32 held : 1;
} RippleElementState;

// returns if the event was consumed
static bool update_element_state(RippleElementState* state, RenderedLayout layout, RippleCursorData cursor_data)
{
    if (cursor_data.x < layout.x || cursor_data.x > layout.x + layout.w ||
        cursor_data.y < layout.y || cursor_data.y > layout.y + layout.h) {
        return false;
    }

    if (cursor_data.left_click) {
        if (state->held || state->clicked) {
            state->clicked = false;
            state->held = true;
        }
        else {
            state->clicked = true;
        }
    }

    return state;
}

// aborts if value is of type SVT_NOT_SPECIFIED
i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value);

// returns or if value is SVT_NOT_SPECIFIED
#define CALCULATE_SIZING_OR(value, relative_member, or) \
    value._type == SVT_NOT_SPECIFIED ? or : \
    apply_relative_sizing(value, get_current_element_parent().relative_member, sum_up_current_element_children().relative_member)

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

Vektor* rendered_layouts = nullptr;
Vektor* elements_layouts = nullptr;
Vektor* parent_element_indices = nullptr;

Mapa* elements_states = nullptr;

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
            .height = config.height,
            .cursor_data = config.cursor_data
    };

    if (elements_layouts) vektor_empty(elements_layouts);
    else elements_layouts = vektor_create(0, sizeof(RippleElementLayoutConfig));

    if (rendered_layouts) vektor_empty(rendered_layouts);
    else rendered_layouts = vektor_create(0, sizeof(RenderedLayout));

    if (parent_element_indices) vektor_empty(parent_element_indices);
    else parent_element_indices = vektor_create(0, sizeof(u32));

    if (!elements_states)
        elements_states = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes);

    // add root element data
    vektor_add(rendered_layouts, &(RenderedLayout) {
        .x = 0,
        .y = 0,
        .w = current_window.width,
        .h = current_window.height
    });
    vektor_add(elements_layouts, &(RippleElementConfig) { });
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

void Ripple_start_element(const char* name, RippleElementConfig config)
{
    // add this element as parent for further child elements
    u32 this_index = vektor_size(rendered_layouts);
    vektor_add(parent_element_indices, &this_index);

    // render out this element so children that are relative to parents still work
    RenderedLayout* rendered_layout = vektor_add(rendered_layouts, &(RenderedLayout){});
    *rendered_layout = render_layout(*rendered_layout, config.layout);
    vektor_add(elements_layouts, &config);

    // debug print
    debug("---------STARTED ELEMENT %s------------", name);
    print_element_config(config);
    print_rendered_layout(*rendered_layout);
}

void Ripple_finish_element(const char* name)
{
    u32 this_index = *(u32*)vektor_last(parent_element_indices);

    // render values that are relative to children
    RenderedLayout *rendered_layout = vektor_get(rendered_layouts, this_index);
    RippleElementConfig config = *(RippleElementConfig*)vektor_get(elements_layouts, this_index);
    *rendered_layout = render_layout(*rendered_layout, config.layout);

    RippleElementState *element_state = &(RippleElementState) {};
    MapaItem* element_state_item = mapa_get_str(elements_states, name);
    if (element_state_item)
        element_state = (RippleElementState*)mapa_get_str(elements_states, name);

    // TODO: this should probably be an event based system, with an array of events and filters an element has
    if (update_element_state(element_state, *rendered_layout, current_window.cursor_data))
    {
        // TODO: consume input
        current_window.cursor_data = (RippleCursorData){};
    }

    if(!element_state_item)
       mapa_insert(elements_states, name, strlen(name), element_state, sizeof(*element_state));

    // TODO: submit all children for rendering


    // delete all children and pop this index
    vektor_pop(parent_element_indices);
    vektor_set_size(rendered_layouts, this_index + 1);
    vektor_set_size(elements_layouts, this_index + 1);

    debug("---------FINISHED ELEMENT %s------------", name);
    print_element_config(config);
    print_rendered_layout(*rendered_layout);
}

void Ripple_submit_element(const char *name, void* user_data)
{
    // submit element for rendering
}

#endif // RIPPLE_IMPLEMENTATION

#ifdef RIPPLE_WIDGETS

#define FOUNDATION ._type = SVT_RELATIVE_PARENT
#define REFINEMENT ._type = SVT_RELATIVE_CHILD
#define DEPTH(value, relation) { ._unsigned_value = (u32)(value * (f32)(2<<14)), relation }
#define FIXED(value) { ._signed_value = value, ._type = SVT_FIXED }

#define RIPPLE_UNIQUE_I_CONCAT(a, b) a##b
#define RIPPLE_UNIQUE_I_PASS(a, b) RIPPLE_UNIQUE_I_CONCAT(a, b)
#define RIPPLE_UNIQUE_I RIPPLE_UNIQUE_I_PASS(i, __LINE__)

#define SURFACE(name, ...) \
    for (u8 RIPPLE_UNIQUE_I = (Ripple_start_window(name, (RippleWindowConfig) { __VA_ARGS__ }), 0); RIPPLE_UNIQUE_I < 1; Ripple_finish_window(name), RIPPLE_UNIQUE_I++)

#define RIPPLE(name, layout, ...) \
    for (u8 RIPPLE_UNIQUE_I = (Ripple_start_element(name, layout), 0); RIPPLE_UNIQUE_I < 1; Ripple_finish_element(name), Ripple_submit_element(name, &__VA_ARGS__), RIPPLE_UNIQUE_I++)

#define IDEA(...) (RippleElementConfig) { __VA_ARGS__ }
#define FORM(...) .layout = { __VA_ARGS__ }

typedef struct {
    Color color;
} RippleButtonConfig;

typedef struct {
    const char* content;
    RippleSizingValue font_size;
} RippleTextConfig;

#define CONSEQUENCE(...) (RippleButtonConfig) { __VA_ARGS__ }

#define PATTERN(...) (RippleTextConfig) { __VA_ARGS__ }

#define ACCEPTANCE  (int){0}

// TODO:
#define TREMBLING() false
#define IS_TREMBLING(name) false

#define INTERACTION() false
#define IS_INTERACTION(name) false

#endif // RIPPLE_WIDGETS
