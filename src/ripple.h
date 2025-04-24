#include <printccy/printccy.h>

#ifndef RIPPLE_NO_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_MAPA

#ifndef RIPPLE_NO_DEFINE_MARROW_VEKTOR
#define MARROW_VEKTOR_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_VEKTOR

#include <marrow/marrow.h>
#include <marrow/vektor.h>
#include <marrow/mapa.h>

#include <string.h>

#ifdef RIPPLE_RENDERING_CUSTOM
#else //RIPPLE_RENDERING_CUSTOM

FILE* ripple_current_window_file = nullptr;

void ripple_render_window_begin(const char* window_name, u32 width, u32 height)
{
    if (ripple_current_window_file)
        fclose(ripple_current_window_file);

    char file_name[255];
    u32 len = print(file_name, 254, "{}.svg", window_name);
    file_name[len] = 0;
    ripple_current_window_file = fopen(file_name, "w");
    printfb(ripple_current_window_file, "<svg xmlns='http://www.w3.org/2000/svg' width='{}' height='{}' style='background:#ffffff'>\n", width, height);
    printfb(ripple_current_window_file, "    <rect x='0' y='0' width='{}' height='{}' fill='#eeeeee'/>\n", width, height);

}

void ripple_render_window_end()
{
    printfb(ripple_current_window_file, "</svg>\n");
    if (ripple_current_window_file)
        fclose(ripple_current_window_file);
    ripple_current_window_file = nullptr;
}

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    char color_str[7] = { [6] = 0 };
    for(i32 i = 0; i < 6; i++) color_str[5 - i] = "0123456789abcdef"[(color >> (i*4)) & 0xf];
    printfb(ripple_current_window_file, "    <rect x='{}' y='{}' width='{}' height='{}' fill='#{}'/>\n", x, y, w, h, color_str);
}

#endif


#undef PRINTCCY_TYPES
#define PRINTCCY_TYPES PRINTCCY_BASE_TYPES, RippleElementConfig: print_element_config, RenderedLayout: print_rendered_layout

int print_element_config(char* output, size_t output_len, va_list* list, const char* args, size_t args_len);
int print_rendered_layout(char* output, size_t output_len, va_list* list, const char* args, size_t args_len);

typedef enum {
    SVT_GROW = 0,
    SVT_FIXED = 1,
    SVT_RELATIVE_CHILD = 2,
    SVT_RELATIVE_PARENT = 3,
} RippleSizingValueType;

typedef struct {
    union{
        u32 _unsigned_value : 30;
        i32 _signed_value : 30;
    };
    RippleSizingValueType _type : 2;
} RippleSizingValue;

typedef u32 Color;

typedef enum {
    cld_HORIZONTAL = 0,
    cld_VERTICAL = 1
} RippleChildLayoutDirection;

typedef struct {
    struct {
        bool fixed : 1;
        RippleChildLayoutDirection child_layout_direction : 1;
    };
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
} RippleElementConfig;

typedef struct {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
    i32 max_w;
    i32 max_h;
} RenderedLayout;

int print_element_config(char* output, size_t output_len, va_list* list, const char* args, size_t args_len) {
    RippleElementConfig config = va_arg(*list, RippleElementConfig);
    #define PRINT_VALUE(name) (i32)config.name._type, config.name._type == SVT_FIXED ? \
                (i64)config.name._signed_value : \
                (f64)config.name._unsigned_value / (f64)(2<<14)
    return print(output, output_len,
           "ElementConfig %{"
                "ElementLayout"
                    " fixed: {}"
                    " x: ({}, {.2f})"
                    " y: ({}, {.2f})"
                    " width: ({}, {.2f})"
                    " height: ({}, {.2f})"
                    " min_width: ({}, {.2f})"
                    " min_height: ({}, {.2f})"
                    " max_width: ({}, {.2f})"
                    " min_width: ({}, {.2f})"
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

typedef void (render_func_t)(RippleElementConfig, RenderedLayout, void*);

void Ripple_start_window(const char* name, RippleWindowConfig config);
void Ripple_finish_window(const char* name);

void Ripple_start_element(const char* name, RippleElementConfig config, render_func_t* render_func);
void Ripple_finish_element(const char *name);

int print_rendered_layout(char* output, size_t output_len, va_list* list, const char* args, size_t args_len)
{
    RenderedLayout layout = va_arg(*list, RenderedLayout);
    return print(output, output_len, "RendredLayout %{ x: {}, y: {}, w: {}, h: {} }",
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
        cursor_data.y < layout.y || cursor_data.y > layout.y + layout.h)
        return false;

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

// aborts if value is of type SVT_GROW
i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value);

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

typedef struct {
    void* data;
} RenderData;

typedef struct {
    const char* name;

    RippleElementConfig config;
    RenderedLayout rendered_layout;
    render_func_t* render_func;

    u32 n_children;
    u32 next_sibling;
} ElementData;

typedef struct {
    RippleCursorData cursor_data;
    u32 width;
    u32 height;
    bool initialized;

    Vektor* elements;
    Vektor* parent_element_indices;
    Mapa* elements_states;
} Window;
Mapa* windows;

thread_local Window current_window;

#define STATE() (RippleElementState) { 0 }
#define STATE_OF(name) (RippleElementState) { 0 }

static RenderedLayout sum_up_current_element_children(void)
{
    RenderedLayout out_layout = {0};
    u32 current_element_index = *(u32*)vektor_last(current_window.parent_element_indices);
    for (u32 i = current_element_index + 1; i < vektor_size(current_window.elements); i++) {
        RenderedLayout child_layout = ((ElementData*)vektor_get(current_window.elements, i))->rendered_layout;
        out_layout.x = min(out_layout.x, child_layout.x);
        out_layout.y = min(out_layout.y, child_layout.y);
        out_layout.w = max(out_layout.w, child_layout.w);
        out_layout.h = max(out_layout.h, child_layout.h);
    }
    return out_layout;
}

i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value)
{
    if (value._type == SVT_GROW)
        abort("SVT_GROW");

    if (value._type == SVT_FIXED)
        return value._signed_value;

    i32 relative_value = value._type == SVT_RELATIVE_PARENT ? parent_value : children_value;
    f32 percentage_value = (f32)value._unsigned_value / (f32)(2<<14);
    return percentage_value * relative_value;
}

void Ripple_start_window(const char* name, RippleWindowConfig config)
{
    if (!windows) windows = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes);

    MapaItem* window = mapa_get(windows, name, sizeof(name));
    current_window = window ?
        *(Window*)window : (Window) {
                    .initialized = true,
                    .width = config.width,
                    .height = config.height,
                    .cursor_data = config.cursor_data
                    };

    if (current_window.elements) vektor_empty(current_window.elements);
    else current_window.elements = vektor_create(0, sizeof(ElementData), nullptr);

    if (current_window.parent_element_indices) vektor_empty(current_window.parent_element_indices);
    else current_window.parent_element_indices = vektor_create(0, sizeof(u32), nullptr);

    if (!current_window.elements_states)
        current_window.elements_states = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes);

    vektor_add(current_window.elements, &(ElementData) {
            .rendered_layout = {
                .x = 0,
                .y = 0,
                .w = current_window.width,
                .h = current_window.height
            },
    });

    // add root element data
    u32 root_index = 0;
    vektor_add(current_window.parent_element_indices, &root_index);

    ripple_render_window_begin(name, current_window.width, current_window.height);

    debug("started window {}", name);
}

static void grow_children(ElementData*);
#define _I1 LINE_UNIQUE_VAR(_i)
#define _for_each_child(el) u32 _I1 = 0; for (ElementData* child = el + 1; _I1 < el->n_children; child = (ElementData*)vektor_get(current_window.elements, child->next_sibling), _I1++ )

// returns or if value is SVT_GROW
#define CALCULATE_SIZING_OR(value, relative_member, or) \
    value._type == SVT_GROW ? or : \
    apply_relative_sizing(value, parent->rendered_layout.relative_member, children_layout.relative_member)

static RenderedLayout render_layout(RenderedLayout this_layout, RippleElementLayoutConfig layout, ElementData* parent)
{
    RenderedLayout children_layout = sum_up_current_element_children();

    this_layout.w = CALCULATE_SIZING_OR(layout.width, w, parent->config.layout.child_layout_direction == cld_HORIZONTAL ? this_layout.w : parent->rendered_layout.w);
    this_layout.max_w = CALCULATE_SIZING_OR(layout.max_width, w, I32_MAX);
    i32 min_width = CALCULATE_SIZING_OR(layout.min_width, w, 0);
    this_layout.w = clamp(this_layout.w, min_width, this_layout.max_w);

    if (layout.x._type != SVT_GROW )
    {
        i32 x = CALCULATE_SIZING_OR(layout.x, w, 0);
        this_layout.x = parent->rendered_layout.x + x;
    }

    this_layout.h = CALCULATE_SIZING_OR(layout.height, h, parent->config.layout.child_layout_direction == cld_VERTICAL ? this_layout.h : parent->rendered_layout.h);
    this_layout.max_h = CALCULATE_SIZING_OR(layout.max_height, h, I32_MAX);
    i32 min_height = CALCULATE_SIZING_OR(layout.min_height, h, 0);
    this_layout.h = clamp(this_layout.h, min_height, this_layout.max_h);

    if (layout.y._type != SVT_GROW )
    {
        i32 y = CALCULATE_SIZING_OR(layout.y, h, 0);
        this_layout.y = parent->rendered_layout.y + y;
    }

    return this_layout;
}

void render_element(ElementData* element)
{
    if(element->render_func) element->render_func(element->config, element->rendered_layout, nullptr);

    grow_children(element);

    _for_each_child(element)
    {
        child->rendered_layout = render_layout(child->rendered_layout, child->config.layout, element);
        render_element(child);
    }
}

void Ripple_finish_window(const char* name)
{
    MapaItem* window = mapa_get(windows, name, sizeof(name));
    if(window) *(Window*)window->data = current_window;
    else mapa_insert(windows, name, sizeof(name), &current_window, sizeof(current_window));

    render_element(vektor_get(current_window.elements, 0));

    vektor_clear(current_window.elements);

    current_window = (Window) {0};

    ripple_render_window_end(name);

    debug("finished window {}", name);
}

static void grow_children(ElementData* data)
{
    if (data->n_children == 0)
        return;

    if (data->config.layout.child_layout_direction == cld_HORIZONTAL)
    {
        u32 free_width = data->rendered_layout.w;
        _for_each_child(data) {
            free_width = free_width >= (u32)child->rendered_layout.w ? free_width - (u32)child->rendered_layout.w : 0;
        }

        while(free_width > 0)
        {
            #define doest_grow_or_fixed_or_max_width child->config.layout.width._type != SVT_GROW || child->config.layout.fixed || child->rendered_layout.w >= child->rendered_layout.max_w

            u32 smallest_width = free_width;
            u32 second_smallest_width = free_width; // others are then resized to this
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_width) continue;

                if ((u32)child->rendered_layout.w < smallest_width)
                {
                    second_smallest_width = smallest_width;
                    smallest_width = (u32)child->rendered_layout.w;
                }
                else if ((u32)child->rendered_layout.w < second_smallest_width && (u32)child->rendered_layout.w != smallest_width)
                    second_smallest_width = (u32)child->rendered_layout.w;

                if ((u32)child->rendered_layout.max_w <= second_smallest_width)
                    second_smallest_width = (u32)child->rendered_layout.max_w;
            }

            u32 n_smallest = 0;
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_width || (u32)child->rendered_layout.w != smallest_width) continue;
                n_smallest += 1;
            }

            if (n_smallest == 0)
                break;

            u32 size_diff = second_smallest_width - smallest_width;
            u32 amount_to_add = min(size_diff, free_width / n_smallest);
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_width || (u32)child->rendered_layout.w != smallest_width) continue;
                child->rendered_layout.w += amount_to_add;
            }

            // distribute the remainder
            u32 remaining_after_rounding = min(size_diff * n_smallest, free_width) - amount_to_add * n_smallest;
            _for_each_child(data) {
                if (remaining_after_rounding-- == 0) break;
                if (doest_grow_or_fixed_or_max_width || (u32)child->rendered_layout.w > second_smallest_width) continue;
                child->rendered_layout.w += 1;
            }

            free_width -= second_smallest_width;
            #undef doest_grow_or_fixed_or_max_width
        }

        // reposition, centers vertically
        u32 offset = 0;
        _for_each_child(data) {
            if (child->config.layout.fixed)
                continue;
            child->rendered_layout.x = data->rendered_layout.x + (i32)offset;
            offset += (u32)child->rendered_layout.w;
            child->rendered_layout.y = data->rendered_layout.y;
        }
    }
    else if (data->config.layout.child_layout_direction == cld_VERTICAL)
    {
        u32 free_height = (u32)data->rendered_layout.h;
        _for_each_child(data) {
            free_height = free_height >= (u32)child->rendered_layout.h ? free_height - (u32)child->rendered_layout.h : 0;
        }

        while(free_height > 0)
        {
            #define doest_grow_or_fixed_or_max_height child->config.layout.height._type != SVT_GROW || child->config.layout.fixed || child->rendered_layout.h >= child->rendered_layout.max_h

            u32 smallest_height = free_height;
            u32 second_smallest_height = free_height; // others are then resized to this
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_height) continue;

                if ((u32)child->rendered_layout.h < smallest_height)
                {
                    second_smallest_height = smallest_height;
                    smallest_height = (u32)child->rendered_layout.h;
                }
                else if ((u32)child->rendered_layout.h < second_smallest_height && (u32)child->rendered_layout.h != smallest_height)
                    second_smallest_height = (u32)child->rendered_layout.h;

                if ((u32)child->rendered_layout.max_h <= second_smallest_height)
                    second_smallest_height = (u32)child->rendered_layout.max_h;
            }

            u32 n_smallest = 0;
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_height || (u32)child->rendered_layout.h != smallest_height) continue;
                n_smallest += 1;
            }

            if (n_smallest == 0)
                break;

            u32 size_diff = second_smallest_height - smallest_height;
            u32 amount_to_add = min(size_diff, free_height / n_smallest);
            _for_each_child(data) {
                if (doest_grow_or_fixed_or_max_height || (u32)child->rendered_layout.h != smallest_height) continue;
                child->rendered_layout.h += amount_to_add;
            }

            // distribute the remainder
            u32 remaining_after_rounding = min(size_diff * n_smallest, free_height) - amount_to_add * n_smallest;
            _for_each_child(data) {
                if (remaining_after_rounding-- == 0) break;
                if (doest_grow_or_fixed_or_max_height || (u32)child->rendered_layout.h != smallest_height) continue;
                child->rendered_layout.h += 1;
            }

            free_height -= second_smallest_height;
            #undef doest_grow_or_fixed_or_max_height
        }

        // reposition, centers horizontally
        u32 offset = 0;
        _for_each_child(data) {
            if (child->config.layout.fixed)
                continue;
            child->rendered_layout.y = data->rendered_layout.y + offset;
            offset += child->rendered_layout.h;
            child->rendered_layout.x = data->rendered_layout.x;
        }
    }
}

void Ripple_start_element(const char* name, RippleElementConfig config, render_func_t* render_func)
{
    // add this element as parent for further child elements
    u32 this_index = vektor_size(current_window.elements);

    u32 parent_element_index = *(u32*)vektor_get(current_window.parent_element_indices, vektor_size(current_window.parent_element_indices) - 1);
    ElementData* parent = vektor_get(current_window.elements, parent_element_index);
    // increment parent element children count and add itself to previous sibling if there is one
    {
        if (parent->n_children++ > 0)
        {
            ElementData* child = parent + 1;
            while (child->next_sibling){ child = vektor_get(current_window.elements, child->next_sibling); }
            child->next_sibling = this_index;
        }
    }

    // is popped in submit function
    vektor_add(current_window.parent_element_indices, &this_index);
    RenderedLayout rendered_layout = render_layout((RenderedLayout){0}, config.layout, parent);

    // render out this element so children that are relative to parents still work
    vektor_add(current_window.elements, &(ElementData){
        .rendered_layout = rendered_layout,
        .config = config,
        .name = name,
        .render_func = render_func
    });

    // debug print
    debug("---------STARTED ELEMENT {}------------", name);
    debug("{}{}\n", config);
    debug("{}\n", rendered_layout);
}

void Ripple_finish_element(const char* name)
{
    u32 this_index = *(u32*)vektor_pop(current_window.parent_element_indices);

    ElementData *data = (ElementData*)vektor_get(current_window.elements, this_index);

    RippleElementState *element_state = &(RippleElementState) {0};
    MapaItem* element_state_item = mapa_get_str(current_window.elements_states, name);
    if (element_state_item)
        element_state = (RippleElementState*)mapa_get_str(current_window.elements_states, name);

    // TODO: make this work
    if (update_element_state(element_state, data->rendered_layout, current_window.cursor_data))
    {
        current_window.cursor_data = (RippleCursorData){0};
    }

    if(!element_state_item)
       mapa_insert(current_window.elements_states, name, strlen(name), element_state, sizeof(*element_state));

    debug("---------FINISHED ELEMENT {}------------", name);
    debug("{}\n", data->config);
    debug("{}\n", data->rendered_layout);
}

#endif // RIPPLE_IMPLEMENTATION

#ifdef RIPPLE_WIDGETS

#define FOUNDATION ._type = SVT_RELATIVE_PARENT
#define REFINEMENT ._type = SVT_RELATIVE_CHILD
#define DEPTH(value, relation) { ._unsigned_value = (u32)(value * (f32)(2<<14)), relation }
#define FIXED(value) { ._signed_value = value, ._type = SVT_FIXED }
#define GROW { ._type = SVT_GROW }

#define SURFACE(name, ...) \
    for (u8 LINE_UNIQUE_I = (Ripple_start_window(name, (RippleWindowConfig) { __VA_ARGS__ }), 0); LINE_UNIQUE_I < 1; Ripple_finish_window(name), LINE_UNIQUE_I++)

#define _RIPPLE_DECL_FUNC(name, ...) \
    void name(RippleElementConfig _c, RenderedLayout _l, void* _p) { __VA_ARGS__ }
#define _RIPPLE_DECL_BODY(name, layout, itername, funcname) \
    for (u8 itername = (Ripple_start_element(name, layout, &funcname), 0); itername < 1; Ripple_finish_element(name), itername++)
#define RIPPLE(name, layout, ...) _RIPPLE_DECL_FUNC(LINE_UNIQUE_VAR(_ripplefunc), __VA_ARGS__); _RIPPLE_DECL_BODY(name, (layout), LINE_UNIQUE_I, LINE_UNIQUE_VAR(_ripplefunc))

#define IDEA(...) (RippleElementConfig) { __VA_ARGS__ }
#define FORM(...) .layout = { __VA_ARGS__ }

typedef struct {
    Color color;
} RippleButtonConfig;

void render_button(RippleElementConfig config, RenderedLayout layout, void* data)
{
    RippleButtonConfig button_data = *(RippleButtonConfig*)data;
    ripple_render_rect(layout.x, layout.y, layout.w, layout.h, button_data.color);
    debug("rendering a button ({} {} {} {}) with a 0x{08X} color", layout.x, layout.y, layout.w, layout.h, (int)button_data.color);
}

#define CONSEQUENCE(...) render_button(_c, _l, &(RippleButtonConfig) { __VA_ARGS__ });

typedef struct {
    const char* content;
    RippleSizingValue font_size;
} RippleTextConfig;

void render_text(RippleElementConfig config, RenderedLayout layout, void* data)
{
    RippleTextConfig text_data = *(RippleTextConfig*)data;
    // TODO: alow some way of getting relative values here !!
    debug("rendering text {} with font size", (char*)text_data.content);
}

#define PATTERN(...) render_text(_c, _l, &(RippleTextConfig) { __VA_ARGS__ });

#define ACCEPTANCE nullptr, nullptr

#define TREMBLING() ( STATE().hovered ? true : false )
#define IS_TREMBLING(name) ( STATE_OF(name).hovered ? true : false )

#endif // RIPPLE_WIDGETS
