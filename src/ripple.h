#ifndef RIPPLE_NO_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_MAPA

#ifndef RIPPLE_NO_DEFINE_MARROW_VEKTOR
#define MARROW_VEKTOR_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_VEKTOR

#include <marrow/marrow.h>

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
    cld_HORIZONTAL,
    cld_VERTICAL
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
    i32 max_w;
    i32 max_h;
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

// aborts if value is of type SVT_GROW
i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value);

// returns or if value is SVT_GROW
#define CALCULATE_SIZING_OR(value, relative_member, or) \
    value._type == SVT_GROW ? or : \
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
    for (u32 i = current_element_index + 1; i < vektor_size(rendered_layouts); i++) {
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
    this_layout.max_w = CALCULATE_SIZING_OR(layout.max_width, w, I32_MAX);
    i32 min_width = CALCULATE_SIZING_OR(layout.min_width, w, 0);
    this_layout.w = clamp(this_layout.w, min_width, this_layout.max_w);

    i32 x = CALCULATE_SIZING_OR(layout.x, w, 0);
    this_layout.x = parent_layout.x + x;

    this_layout.h = CALCULATE_SIZING_OR(layout.height, h, parent_layout.h);
    this_layout.max_h = CALCULATE_SIZING_OR(layout.max_height, h, I32_MAX);
    i32 min_height = CALCULATE_SIZING_OR(layout.min_height, h, 0);
    this_layout.h = clamp(this_layout.h, min_height, this_layout.max_h);

    i32 y = CALCULATE_SIZING_OR(layout.y, h, 0);
    this_layout.y = parent_layout.y + y;

    return this_layout;
}

static void print_all_rendered_layouts()
{
    for (u32 i = 0; i < vektor_size(rendered_layouts); i++) {
        print_rendered_layout(*(RenderedLayout*)vektor_get(rendered_layouts, i));
    }
}

static void apply_child_direction(RippleElementLayoutConfig config, RenderedLayout layout, u32 n_children)
{
    if (n_children == 0)
        return;

    RenderedLayout *child_layouts = (RenderedLayout*)vektor_get(rendered_layouts, vektor_size(rendered_layouts) - n_children);
    RippleElementConfig *child_configs = (RippleElementConfig*)vektor_get(elements_layouts, vektor_size(rendered_layouts) - n_children);

    if (config.child_layout_direction == cld_HORIZONTAL)
    {
        u32 free_width = layout.w;
        for_each(child_layout, child_layouts, n_children) {
            free_width = free_width >= (u32)child_layout.w ? free_width - (u32)child_layout.w : 0;
        }

        while(free_width > 0)
        {
            #define doest_grow_or_fixed_or_max_width child_configs[i].layout.width._type != SVT_GROW || child_configs[i].layout.fixed || child_layout.w >= child_layout.max_w

            u32 smallest_width = free_width;
            u32 second_smallest_width = free_width; // others are then resized to this
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_width) continue;

                if ((u32)child_layout.w <= smallest_width)
                {
                    second_smallest_width = smallest_width;
                    smallest_width = (u32)child_layout.w;
                }
                else if ((u32)child_layout.w < second_smallest_width)
                    second_smallest_width = (u32)child_layout.w;

                if ((u32)child_layout.max_w <= second_smallest_width)
                    second_smallest_width = (u32)child_layout.max_w;
            }

            u32 n_smallest = 0;
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_width || (u32)child_layout.w != smallest_width) continue;
                n_smallest += 1;
            }

            u32 size_diff = second_smallest_width - smallest_width;
            u32 amount_to_add = min(size_diff, free_width / n_smallest);
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_width || (u32)child_layout.w != smallest_width) continue;
                child_layout.w += amount_to_add;
            }

            // distribute the remainder
            u32 remaining_after_rounding = min(size_diff * n_smallest, free_width) - amount_to_add * n_smallest;
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (remaining_after_rounding-- == 0) break;
                if (doest_grow_or_fixed_or_max_width || (u32)child_layout.w != smallest_width) continue;
                child_layout.w += 1;
            }

            free_width -= second_smallest_width;
            #undef doest_grow_or_fixed_or_max_width
        }

        // reposition, centers vertically
        u32 offset = 0;
        for_each_i(child_layout, child_layouts, n_children, i) {
            if (child_configs[i].layout.fixed)
                continue;
            child_layout.x = layout.x + (i32)offset;
            offset += (u32)child_layout.w;
            child_layout.y = layout.y + (layout.h - child_layout.h ) / 2;
        }
    }

    if (config.child_layout_direction == cld_VERTICAL)
    {
        u32 free_height = (u32)layout.h;
        for_each(child_layout, child_layouts, n_children) {
            free_height = free_height >= (u32)child_layout.h ? free_height - (u32)child_layout.h : 0;
        }

        while(free_height > 0)
        {
            #define doest_grow_or_fixed_or_max_height child_configs[i].layout.height._type != SVT_GROW || child_configs[i].layout.fixed || child_layout.h >= child_layout.max_h

            u32 smallest_height = free_height;
            u32 second_smallest_height = free_height; // others are then resized to this
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_height) continue;

                if ((u32)child_layout.h <= smallest_height)
                {
                    second_smallest_height = smallest_height;
                    smallest_height = (u32)child_layout.h;
                }
                else if ((u32)child_layout.h < second_smallest_height)
                    second_smallest_height = (u32)child_layout.h;

                if ((u32)child_layout.max_h <= second_smallest_height)
                    second_smallest_height = (u32)child_layout.max_h;
            }

            u32 n_smallest = 0;
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_height || (u32)child_layout.h != smallest_height) continue;
                n_smallest += 1;
            }

            u32 size_diff = second_smallest_height - smallest_height;
            u32 amount_to_add = min(size_diff, free_height / n_smallest);
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (doest_grow_or_fixed_or_max_height || (u32)child_layout.h != smallest_height) continue;
                child_layout.h += amount_to_add;
            }

            // distribute the remainder
            u32 remaining_after_rounding = min(size_diff * n_smallest, free_height) - amount_to_add * n_smallest;
            for_each_i(child_layout, child_layouts, n_children, i) {
                if (remaining_after_rounding-- == 0) break;
                if (doest_grow_or_fixed_or_max_height || (u32)child_layout.h != smallest_height) continue;
                child_layout.h += 1;
            }

            free_height -= second_smallest_height;
            #undef doest_grow_or_fixed_or_max_height
        }

        // reposition, centers horizontally
        u32 offset = 0;
        for_each_i(child_layout, child_layouts, n_children, i) {
            if (child_configs[i].layout.fixed)
                continue;
            child_layout.y = layout.y + offset;
            offset += child_layout.h;
            child_layout.x = layout.x + (layout.h - child_layout.h) / 2;
        }
    }
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

    RenderedLayout *rendered_layout = vektor_get(rendered_layouts, this_index);
    RippleElementConfig config = *(RippleElementConfig*)vektor_get(elements_layouts, this_index);

    // reorder children based on layout
    apply_child_direction(config.layout, *rendered_layout, vektor_size(rendered_layouts) - this_index - 1);

    // render values that are relative to children
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

#define SURFACE(name, ...) \
    for (u8 LINE_UNIQUE_I = (Ripple_start_window(name, (RippleWindowConfig) { __VA_ARGS__ }), 0); LINE_UNIQUE_I < 1; Ripple_finish_window(name), LINE_UNIQUE_I++)

#define RIPPLE(name, layout, ...) \
    for (u8 LINE_UNIQUE_I = (Ripple_start_element(name, layout), 0); LINE_UNIQUE_I < 1; Ripple_finish_element(name), Ripple_submit_element(name, &__VA_ARGS__), LINE_UNIQUE_I++)

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
