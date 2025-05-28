#ifndef RIPPLE_H
#define RIPPLE_H

// for offsetof
#include <stddef.h>

#include <printccy/printccy.h>

#ifndef RIPPLE_NO_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_MAPA

#ifndef RIPPLE_NO_DEFINE_MARROW_VEKTOR
#define MARROW_VEKTOR_IMPLEMENTATION
#endif // RIPPLE_NO_DEFINE_MARROW_VEKTOR

#include <marrow/marrow.h>
#include <marrow/vektor.h>
#include <marrow/allocator.h>
#include <marrow/mapa.h>

#include <string.h>

typedef struct {
    struct {
        u8 initialized : 1;
        u8 should_close : 1;
    };
} RippleWindowState;

typedef struct {
    struct {
        u32 left_pressed : 1;
        u32 right_pressed : 1;
        u32 middle_pressed : 1;
    };

    i32 x;
    i32 y;
} RippleCursorState;

typedef struct {
    const char* title;

    u32 width;
    u32 height;

    struct {
        u32 not_resizable : 1;
    };

    Allocator* allocator;
} RippleWindowConfig;

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

typedef struct {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
    i32 max_w;
    i32 max_h;
} RenderedLayout;

typedef enum {
    cld_HORIZONTAL = 0,
    cld_VERTICAL = 1
} RippleChildLayoutDirection;

typedef struct {
    struct {
        bool fixed : 1;
        RippleChildLayoutDirection direction : 1;
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

struct RippleElementConfig;
typedef void (render_func_t)(struct RippleElementConfig, RenderedLayout);

typedef struct RippleElementConfig {
    RippleElementLayoutConfig layout;
    struct{
        bool accept_input : 1;
        bool _dummy : 1;
    };

    render_func_t* render_func;
    void* render_data;
    usize render_data_size;
} RippleElementConfig;

typedef struct {
    u32 clicked : 1;
    u32 hovered : 1;
    u32 held : 1;
} RippleElementState;

typedef u32 RippleColor;

void Ripple_start_window(RippleWindowConfig config);
void Ripple_finish_window(void);

void Ripple_push_id(u64);
void Ripple_submit_element(RippleElementConfig config); // copies render data if its size is non zero
void Ripple_pop_id(void);

void Ripple_begin(Allocator* allocator);
void Ripple_end(void);

// backend functions
void ripple_render_window_begin(RippleWindowConfig config);
void ripple_get_window_size(u32* width, u32* height);
RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config);
RippleCursorState ripple_update_cursor_state(RippleCursorState state);
void ripple_render_window_end(RippleWindowConfig config);
void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color);

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

typedef struct {
    struct {
        u32 frame_color : 1;
    };
    RippleElementState state;
    RenderedLayout layout;
} ElementState;

typedef struct {
    u64 id;
    RippleElementConfig config;
    RenderedLayout calculated_layout;

    u32 parent_element;
    u32 n_children;
    u32 next_sibling;
} ElementData;

typedef struct {
    RippleWindowConfig config;
    RippleWindowState state;
    RippleCursorState cursor_state;
    Vektor* elements;
    Mapa* elements_states;

    struct {
        u32 frame_color : 1;
    };

    struct {
        u64 id;
        u32 index;
    } current_element;
} Window;
Mapa* windows;

static thread_local Window current_window;

void Ripple_start_window(RippleWindowConfig config)
{
    if (!windows) windows = mapa_create(mapa_hash_MurmurOAAT_32, mapa_cmp_bytes, config.allocator);

    MapaItem* window = mapa_get(windows, config.title, sizeof(config.title));
    current_window = window ? *(Window*)window->data : (Window) { 0 };

    current_window.config = config;

    if (current_window.elements) vektor_empty(current_window.elements);
    else current_window.elements = vektor_create(0, sizeof(ElementData), nullptr);

    if (!current_window.elements_states)
        current_window.elements_states = mapa_create(mapa_hash_u64, mapa_cmp_bytes, config.allocator);

    ripple_render_window_begin(current_window.config);

    ripple_get_window_size(&current_window.config.width, &current_window.config.height);
    current_window.cursor_state = ripple_update_cursor_state(current_window.cursor_state);

    vektor_add(current_window.elements, &(ElementData) {
            .calculated_layout = {
                .x = 0,
                .y = 0,
                .w = current_window.config.width,
                .h = current_window.config.height
            },
    });

    current_window.current_element.index = 0;
    current_window.current_element.id = 0;

    current_window.frame_color++;
}

static void submit_element(ElementData* element);

void Ripple_finish_window(void)
{
    current_window.state = ripple_update_window_state(current_window.state, current_window.config);

    submit_element(vektor_get(current_window.elements, 0));

    ripple_render_window_end(current_window.config);

    // remove dead elements
    {
        u32 n_elements = mapa_capacity(current_window.elements_states);
        for (mapa_size_t element_i = 0; element_i < n_elements; element_i++)
        {
            MapaItem* item = mapa_get_at_index(current_window.elements_states, element_i);
            ElementState* state = (ElementState*)item->data;

            if (state && state->frame_color != current_window.frame_color)
                mapa_remove_at_index(current_window.elements_states, element_i);
        }
    }

    MapaItem* window_item = mapa_get(windows, current_window.config.title, sizeof(current_window.config.title));
    if (window_item) *(Window*)window_item->data = current_window;
    else mapa_insert(windows, current_window.config.title, sizeof(current_window.config.title), &current_window, sizeof(current_window));
}

static i64 apply_relative_sizing(RippleSizingValue value, i32 parent_value, i32 children_value)
{
    if (value._type == SVT_FIXED)
        return value._signed_value;

    i32 relative_value = value._type == SVT_RELATIVE_PARENT ? parent_value : children_value;
    f32 percentage_value = (f32)value._unsigned_value / (f32)(2<<14);
    return percentage_value * relative_value;
}

// returns or if value is SVT_GROW
#define CALCULATE_SIZING_OR(value, relative_member, or) \
    value._type == SVT_GROW ? or : \
    apply_relative_sizing(value, parent->calculated_layout.relative_member, children_layout.relative_member)

static RenderedLayout sum_up_current_element_children(void)
{
    RenderedLayout out_layout = {0};
    for (u32 i = current_window.current_element.index + 1; i < vektor_size(current_window.elements); i++) {
        RenderedLayout child_layout = ((ElementData*)vektor_get(current_window.elements, i))->calculated_layout;
        out_layout.x = min(out_layout.x, child_layout.x);
        out_layout.y = min(out_layout.y, child_layout.y);
        out_layout.w = max(out_layout.w, child_layout.w);
        out_layout.h = max(out_layout.h, child_layout.h);
    }
    return out_layout;
}

static RenderedLayout calculate_layout(RippleElementLayoutConfig layout, RenderedLayout this_layout, ElementData* parent)
{
    RenderedLayout children_layout = sum_up_current_element_children();

    this_layout.w = CALCULATE_SIZING_OR(layout.width, w, parent->config.layout.direction == cld_HORIZONTAL ? this_layout.w : parent->calculated_layout.w);
    this_layout.max_w = CALCULATE_SIZING_OR(layout.max_width, w, I32_MAX);
    i32 min_width = CALCULATE_SIZING_OR(layout.min_width, w, 0);
    this_layout.w = clamp(this_layout.w, min_width, this_layout.max_w);

    if (layout.x._type != SVT_GROW )
    {
        i32 x = CALCULATE_SIZING_OR(layout.x, w, 0);
        this_layout.x = parent->calculated_layout.x + x;
    }

    this_layout.h = CALCULATE_SIZING_OR(layout.height, h, parent->config.layout.direction == cld_VERTICAL ? this_layout.h : parent->calculated_layout.h);
    this_layout.max_h = CALCULATE_SIZING_OR(layout.max_height, h, I32_MAX);
    i32 min_height = CALCULATE_SIZING_OR(layout.min_height, h, 0);
    this_layout.h = clamp(this_layout.h, min_height, this_layout.max_h);

    if (layout.y._type != SVT_GROW )
    {
        i32 y = CALCULATE_SIZING_OR(layout.y, h, 0);
        this_layout.y = parent->calculated_layout.y + y;
    }

    return this_layout;
}

static void update_element_state(u64 element_id)
{
    MapaItem* element_state_item = mapa_get(current_window.elements_states, &element_id, sizeof(element_id));
    if (!element_state_item) // we dont update new elements yet, they get added in their finish function
        return;

    ElementState *state = (ElementState*)element_state_item->data;

    state->frame_color = current_window.frame_color;

    state->state.hovered =
        current_window.cursor_state.x >= state->layout.x && current_window.cursor_state.x < state->layout.x + state->layout.w &&
        current_window.cursor_state.y >= state->layout.y && current_window.cursor_state.y < state->layout.y + state->layout.h;

    state->state.clicked = state->state.hovered && current_window.cursor_state.left_pressed;
}

#define _I1 LINE_UNIQUE_VAR(_i)
#define _for_each_child(el) u32 _I1 = 0; for (ElementData* child = el + 1; _I1 < el->n_children; child = (ElementData*)vektor_get(current_window.elements, child->next_sibling), _I1++ )

static void grow_children(ElementData* data)
{
    if (data->n_children == 0)
        return;

    u32 dim_offsets[] = { offsetof(RenderedLayout, w), offsetof(RenderedLayout, h) };
    u32 max_dim_offsets[] = { offsetof(RenderedLayout, max_w), offsetof(RenderedLayout, max_h) };
    u32 pos_offsets[] = { offsetof(RenderedLayout, x), offsetof(RenderedLayout, y) };
    u32 other_pos_offsets[] = { offsetof(RenderedLayout, y), offsetof(RenderedLayout, x) };
    u32 layout_dim_offsets[] = { offsetof(RippleElementLayoutConfig, width), offsetof(RippleElementLayoutConfig, height) };
    #define DIM(arg) (*(i32*)((u8*)(&arg) + dim_offsets[(u32)data->config.layout.direction]))
    #define MAX_DIM(arg) (*(i32*)((u8*)(&arg) + max_dim_offsets[(u32)data->config.layout.direction]))
    #define POS(arg) (*(i32*)((u8*)(&arg) + pos_offsets[(u32)data->config.layout.direction]))
    #define OTHER_POS(arg) (*(i32*)((u8*)(&arg) + other_pos_offsets[(u32)data->config.layout.direction]))
    #define LAYOUT_DIM(arg) (*(RippleSizingValue*)((u8*)(&arg) + layout_dim_offsets[(u32)data->config.layout.direction]))

    u32 free_space = (u32)DIM(data->calculated_layout);
    _for_each_child(data) {
        free_space = free_space >= (u32)DIM(child->calculated_layout) ? free_space - (u32)DIM(child->calculated_layout) : 0;
    }

    while(free_space > 0)
    {
        #define doest_grow_or_fixed_or_max_width (LAYOUT_DIM(child->config.layout))._type != SVT_GROW || child->config.layout.fixed || DIM(child->calculated_layout) >= MAX_DIM(child->calculated_layout)

        u32 smallest_width = free_space;
        u32 second_smallest_width = free_space; // others are then resized to this
        _for_each_child(data) {
            if (doest_grow_or_fixed_or_max_width) continue;

            if ((u32)DIM(child->calculated_layout) < smallest_width)
            {
                second_smallest_width = smallest_width;
                smallest_width = (u32)DIM(child->calculated_layout);
            }
            else if ((u32)DIM(child->calculated_layout) < second_smallest_width && ((u32)DIM(child->calculated_layout)) != smallest_width)
                second_smallest_width = (u32)DIM(child->calculated_layout);

            if ((u32)MAX_DIM(child->calculated_layout) <= second_smallest_width)
                second_smallest_width = (u32)MAX_DIM(child->calculated_layout);
        }

        u32 n_smallest = 0;
        _for_each_child(data) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            n_smallest += 1;
        }

        if (n_smallest == 0)
            break;

        u32 size_diff = second_smallest_width - smallest_width;
        u32 amount_to_add = min(size_diff, free_space / n_smallest);
        _for_each_child(data) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            DIM(child->calculated_layout) += amount_to_add;
        }

        smallest_width += amount_to_add;

        // distribute the remainder
        u32 remaining_after_rounding = min(size_diff * n_smallest, free_space) - amount_to_add * n_smallest;
        _for_each_child(data) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            if (remaining_after_rounding-- == 0) break;
            child->calculated_layout.w += 1;
        }

        free_space -= second_smallest_width;
        #undef doest_grow_or_fixed_or_max_width
    }

    // reposition, centers on other dimension
    u32 offset = 0;
    _for_each_child(data) {
        if (child->config.layout.fixed)
            continue;
        POS(child->calculated_layout) = POS(data->calculated_layout) + offset;
        offset += DIM(child->calculated_layout);
        OTHER_POS(child->calculated_layout) = OTHER_POS(data->calculated_layout);
    }

    #undef DIM
    #undef MAX_DIM
    #undef POS
    #undef OTHER_POS
    #undef LAYOUT_DIM
}

// submits element for rendering and calculates its input state
static void submit_element(ElementData* element)
{
    if (element->config.render_func) element->config.render_func(element->config, element->calculated_layout);
    allocator_free(current_window.config.allocator, element->config.render_data, element->config.render_data_size);

    MapaItem* state_item = mapa_get(current_window.elements_states, &element->id, sizeof(element->id));
    if (state_item)
    {
        ((ElementState*)state_item->data)->layout = element->calculated_layout;
    }
    else
    {
        mapa_insert(current_window.elements_states, &element->id, sizeof(element->id), &(ElementState){ .layout = element->calculated_layout }, sizeof(ElementState));
    }

    update_element_state(element->id);

    // our layout is done and we can calculate children
    _for_each_child(element)
    {
        child->calculated_layout = calculate_layout(child->config.layout, child->calculated_layout, element);
    }

    grow_children(element);

    _for_each_child(element)
    {
        submit_element(child);
    }
}

#undef _for_each_child

static u64 generate_element_id(u64 base, u64 parent_id, u32 index)
{
    return hash_u64(hash_combine(base, hash_u64(parent_id + (u64)index * 1000)));
}

// should be called before submit element
void Ripple_push_id(u64 id)
{
    ElementData* parent = vektor_get(current_window.elements, current_window.current_element.index);
    current_window.current_element.id = id ? generate_element_id(id, current_window.current_element.id, parent->n_children) : id;
}

void Ripple_submit_element(RippleElementConfig config)
{
    u32 parent_element_index = current_window.current_element.index;
    current_window.current_element.index = vektor_size(current_window.elements);

    ElementData* parent = vektor_get(current_window.elements, parent_element_index);
    if (parent->n_children++ > 0)
    {
        ElementData* child = parent + 1;
        while (child->next_sibling) { child = vektor_get(current_window.elements, child->next_sibling); }
        child->next_sibling = current_window.current_element.index;
    }

    RenderedLayout calculated_layout = calculate_layout(config.layout, (RenderedLayout){0}, parent);

    // render_data is supposed to be set if render_data_size is also
    if (config.render_data_size)
        config.render_data = allocator_make_copy(current_window.config.allocator, config.render_data, config.render_data_size);

    vektor_add(current_window.elements, &(ElementData){
        .calculated_layout = calculated_layout,
        .config = config,
        .parent_element = parent_element_index,
        .id = current_window.current_element.id
    });
}

void Ripple_pop_id(void)
{
    ElementData *data = (ElementData*)vektor_get(current_window.elements, current_window.current_element.index);
    ElementData *parent = (ElementData*)vektor_get(current_window.elements, data->parent_element);
    current_window.current_element.index = data->parent_element;
    current_window.current_element.id = parent->id;
}

void Ripple_begin(Allocator* allocator)
{
}

void Ripple_end(void)
{

}

#endif // RIPPLE_IMPLEMENTATION


#ifdef RIPPLE_WIDGETS

static RippleElementState _get_element_state(u64 id)
{
    MapaItem* item = mapa_get(current_window.elements_states, &id, sizeof(id));
    return item ? ((ElementState*)item->data)->state : (RippleElementState){ 0 };
}

#define STATE() (_get_element_state(current_window.current_element.id))

#define FOUNDATION ._type = SVT_RELATIVE_PARENT
#define REFINEMENT ._type = SVT_RELATIVE_CHILD
#define DEPTH(value, relation) { ._unsigned_value = (u32)((value) * (f32)(2<<14)), relation }
#define FIXED(value) { ._signed_value = value, ._type = SVT_FIXED }
#define GROW { ._type = SVT_GROW }

#define SURFACE(...) for (u8 LINE_UNIQUE_I = (Ripple_start_window((RippleWindowConfig) { __VA_ARGS__ }), 0); LINE_UNIQUE_I < 1; Ripple_finish_window(), LINE_UNIQUE_I++)

#define SURFACE_IS_STABLE(...) ( !current_window.state.should_close )

#define RIPPLE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (Ripple_push_id(LINE_UNIQUE_HASH), Ripple_submit_element((RippleElementConfig) { ._dummy = 0, __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; Ripple_pop_id(), LINE_UNIQUE_VAR(_rippleiter)++)

#define FORM(...) .layout = { __VA_ARGS__ }

#define OPEN_THE_VOID(allocator) Ripple_begin(allocator)
#define CLOSE_THE_VOID() Ripple_end()

typedef struct {
    RippleColor color;
} RippleRectangleConfig;

void render_rectangle(RippleElementConfig config, RenderedLayout layout)
{
    RippleRectangleConfig rectangle_data = *(RippleRectangleConfig*)config.render_data;
    ripple_render_rect(layout.x, layout.y, layout.w, layout.h, rectangle_data.color);
}

#define RECTANGLE(...)\
  .render_func = render_rectangle,\
  .render_data = &(RippleRectangleConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleRectangleConfig)

#define CENTERED_HORIZONTAL(...) do {\
        RIPPLE();\
        { __VA_ARGS__ }\
        RIPPLE();\
} while (false)
#define CENTERED_VERTICAL(...) do {\
        RIPPLE( FORM( .direction = cld_VERTICAL ) ) {\
            RIPPLE();\
            { __VA_ARGS__ }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED(...) CENTERED_HORIZONTAL(CENTERED_VERTICAL(__VA_ARGS__);)
#endif // RIPPLE_WIDGETS

#endif // RIPPLE_H
