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

typedef struct {
    u8 initialized : 1;
    u8 is_open : 1;
} RippleWindowState;

typedef struct {
    u8 pressed : 1;
    u8 released : 1;
    u8 held: 1;
} MouseButtonState;

typedef struct {
    MouseButtonState left;
    MouseButtonState right;
    MouseButtonState middle;
    i32 x;
    i32 y;
} RippleCursorState;

typedef struct {
    const char* title;
    bool* is_open;
    Allocator* allocator;

    i32* x;
    i32* y;

    u32 width;
    u32 height;

    struct {
        bool not_resizable : 1;
        bool hide_title : 1;
        bool set_position : 1;
    };
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
    RippleSizingValue x;
    RippleSizingValue y;
    RippleSizingValue width;
    RippleSizingValue height;
    RippleSizingValue min_width;
    RippleSizingValue min_height;
    RippleSizingValue max_width;
    RippleSizingValue max_height;
    struct {
        bool fixed : 1;
        RippleChildLayoutDirection direction : 1;
    };
} RippleElementLayoutConfig;

struct RippleElementConfig;
typedef void (render_func_t)(struct RippleElementConfig, RenderedLayout, void*, RippleRenderData);

typedef struct RippleElementConfig {
    RippleElementLayoutConfig layout;

    render_func_t* render_func;
    void* render_data;
    usize render_data_size;

    struct{
        bool accept_input : 1;
        bool _dummy : 1;
    };
} RippleElementConfig;

typedef struct {
    u8 clicked : 1;
    u8 hovered : 1;
    u8 is_held : 1;
    u8 is_weak_held : 1;
} RippleElementState;

typedef u32 RippleColor;

bool Ripple_start_window(RippleWindowConfig config);
void Ripple_finish_window(void);

void Ripple_push_id(u64);
void Ripple_submit_element(RippleElementConfig config); // copies render data if its size is non zero
void Ripple_pop_id(void);

void Ripple_begin(Allocator* allocator);
void Ripple_end(void);

RippleRenderData Ripple_render_begin();
void Ripple_render_end(RippleRenderData user_data);

// backend functions
void ripple_backend_initialize(RippleBackendConfig config);
RippleBackendConfig ripple_get_default_backend_config();

void ripple_window_begin(u64 id, RippleWindowConfig config);
void ripple_window_end();
void ripple_window_close(u64 id);
void ripple_get_window_size(u32* width, u32* height);
RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config);
RippleCursorState ripple_update_cursor_state(RippleCursorState state);

RippleRenderData ripple_render_begin();
    void ripple_render_window_begin(u64, RippleRenderData);
        void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color);
        void ripple_render_image(i32 x, i32 y, i32 w, i32 h, RippleImage color);
        void ripple_render_text(i32 x, i32 y, const char* text, f32 font_size, u32 color);
        void ripple_measure_text(const char* text, f32 font_size, i32* out_w, i32* out_h);
    void ripple_render_window_end(RippleRenderData);
void ripple_render_end(RippleRenderData);

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

typedef struct {
    RenderedLayout layout;
    RippleElementState state;
    struct {
        u8 frame_color : 1;
    };
} ElementState;

typedef struct {
    u64 id;
    RippleElementConfig config;
    RenderedLayout calculated_layout;

    RenderedLayout children_bounds;

    bool update_state;

    u32 parent_element;
    u32 n_children;
    u32 last_child;
    u32 next_sibling;
} ElementData;

typedef struct {
    u64 id;
    u64 parent_id;
    RippleWindowConfig config;
    RippleWindowState state;
    RippleCursorState cursor_state;

    struct {
        u8 frame_color : 1;
    };

    VEKTOR(ElementData) elements;
    MAPA(u64, ElementState) elements_states;
    struct {
        u64 id;
        u32 index;
    } current_element;

    void* user_data;
} Window;

static thread_local MAPA(u64, Window) windows = { 0 };
static thread_local Window* current_window = nullptr;
static thread_local u32 current_frame_color = 0;

bool Ripple_start_window(RippleWindowConfig config)
{
    if (!windows.entries)
        mapa_init(windows, mapa_hash_u64, mapa_cmp_bytes, config.allocator);

    u64 parent_id = current_window ? current_window->id : 0;

    u64 window_id = hash_str(config.title);
    current_window = mapa_get(windows, &window_id);
    if (!current_window)
        current_window = mapa_insert(windows, &window_id, (Window){ .id = window_id });

    current_window->parent_id = parent_id;

    current_window->config = config;

    // update backend and state
    {
        ripple_window_begin(window_id, config);

        ripple_get_window_size(&current_window->config.width, &current_window->config.height);

        current_window->state = ripple_update_window_state(current_window->state, current_window->config);

        if (config.is_open) *config.is_open = current_window->state.is_open;

        current_window->cursor_state.left.held |= current_window->cursor_state.left.pressed;
        current_window->cursor_state.right.held |= current_window->cursor_state.right.pressed;
        current_window->cursor_state.middle.held |= current_window->cursor_state.middle.pressed;

        current_window->cursor_state = ripple_update_cursor_state(current_window->cursor_state);

        if (current_window->cursor_state.left.released) current_window->cursor_state.left.held = 0;
        if (current_window->cursor_state.right.released) current_window->cursor_state.right.held = 0;
        if (current_window->cursor_state.middle.released) current_window->cursor_state.middle.held = 0;
    }

    // reset window elements
    {
        if (current_window->elements.items) vektor_clear(current_window->elements);
        else vektor_init(current_window->elements, 0, config.allocator);

        if (!current_window->elements_states.entries)
            mapa_init(current_window->elements_states, mapa_hash_u64, mapa_cmp_bytes, config.allocator);

        vektor_add(current_window->elements, (ElementData) {
                .calculated_layout = {
                    .x = 0,
                    .y = 0,
                    .w = current_window->config.width,
                    .h = current_window->config.height
                },
        });
        current_window->current_element.id = 0;
        current_window->current_element.index = 0;

        current_window->frame_color = current_frame_color;
    }

    return current_window->state.is_open;
}

void Ripple_finish_window()
{
    ripple_window_end();
    current_window = current_window->parent_id ?
        mapa_get(windows, &current_window->parent_id) :
        nullptr;
}

static void finalize_element(Window* window, ElementData* element);

static void render_element(Window* window, ElementData* element, void* window_user_data, RippleRenderData user_data);

static void update_window(Window* window)
{
    finalize_element(window, &window->elements.items[0]);

    // remove dead elements
    for (i32 element_i = 0; element_i < (i64)window->elements_states.size; element_i++)
    {
        ElementState* state = mapa_get_at_index(window->elements_states, (u64)element_i);
        if(!state) continue;

        if (state->frame_color != window->frame_color)
        {
            mapa_remove_at_index(window->elements_states, (u64)element_i);
            element_i--;
            continue;
        }
    }
}

static void close_window(Window* window)
{
    ripple_window_close(window->id);
    vektor_free(window->elements);
    mapa_free(window->elements_states);
}

RippleRenderData Ripple_render_begin()
{
    for (i32 window_i = 0; window_i < (i64)windows.size; window_i++)
    {
        Window* window = mapa_get_at_index(windows, (u64)window_i);
        if(!window) continue;

        if (window->frame_color != current_frame_color)
        {
            close_window(window);
            mapa_remove_at_index(windows, (u64)window_i);
            window_i--;
            continue;
        }

        update_window(window);
    }

    current_frame_color = current_frame_color ? 0 : 1;

    return ripple_render_begin();
}

void Ripple_render_end(RippleRenderData render_data)
{
    for (u32 window_i = 0; window_i < windows.size; window_i++)
    {
        Window* window = mapa_get_at_index(windows, window_i);
        if (!window) continue;

        ripple_render_window_begin(window->id, render_data);
        if (window->elements.n_items){
            render_element(window, &window->elements.items[0], window->user_data, render_data);
        }
        ripple_render_window_end(render_data);
    }

    ripple_render_end(render_data);
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
    apply_relative_sizing(value, parent->calculated_layout.relative_member, element->children_bounds.relative_member)

static RenderedLayout calculate_layout(ElementData* element, ElementData* parent)
{
    RippleElementLayoutConfig config = element->config.layout;
    RenderedLayout layout = element->calculated_layout;

    layout.w = CALCULATE_SIZING_OR(config.width, w, parent->config.layout.direction == cld_HORIZONTAL ? layout.w : parent->calculated_layout.w);
    layout.max_w = CALCULATE_SIZING_OR(config.max_width, w, I32_MAX);
    i32 min_width = CALCULATE_SIZING_OR(config.min_width, w, 0);
    layout.w = clamp(layout.w, min_width, layout.max_w);

    if (config.x._type != SVT_GROW )
    {
        i32 x = CALCULATE_SIZING_OR(config.x, w, 0);
        layout.x = parent->calculated_layout.x + x;
    }

    layout.h = CALCULATE_SIZING_OR(config.height, h, parent->config.layout.direction == cld_VERTICAL ? layout.h : parent->calculated_layout.h);
    layout.max_h = CALCULATE_SIZING_OR(config.max_height, h, I32_MAX);
    i32 min_height = CALCULATE_SIZING_OR(config.min_height, h, 0);
    layout.h = clamp(layout.h, min_height, layout.max_h);

    if (config.y._type != SVT_GROW )
    {
        i32 y = CALCULATE_SIZING_OR(config.y, h, 0);
        layout.y = parent->calculated_layout.y + y;
    }

    return layout;
}

#undef CALCULATE_SIZING_OR

static void update_element_state(Window* window, ElementState* state)
{
    state->state.hovered =
        window->cursor_state.x >= state->layout.x && window->cursor_state.x < state->layout.x + state->layout.w &&
        window->cursor_state.y >= state->layout.y && window->cursor_state.y < state->layout.y + state->layout.h;

    state->state.is_held = window->cursor_state.left.held && (state->state.clicked || (state->state.is_held && state->state.hovered));
    state->state.is_weak_held = window->cursor_state.left.held && (state->state.clicked || state->state.is_weak_held);
    state->state.clicked = state->state.hovered && window->cursor_state.left.pressed;
}

#define _I1 LINE_UNIQUE_VAR(_i)
#define _for_each_child(el) u32 _I1 = 0; for (ElementData* child = el + 1; _I1 < el->n_children; child = &window->elements.items[child->next_sibling], _I1++ )

static void grow_children(Window* window, ElementData* data)
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

// recursively renderes elements
static void render_element(Window* window, ElementData* element, void* window_user_data, RippleRenderData render_data)
{
    if (element->config.render_func)
        element->config.render_func(element->config, element->calculated_layout, window_user_data, render_data);

    allocator_free(window->config.allocator, element->config.render_data, element->config.render_data_size);

    _for_each_child(element)
    {
        render_element(window, child, window_user_data, render_data);
    }
}

// recursively grows child elements, this is the last step in calculating element layouts
// for convenience also updates element state with new layout
static void finalize_element(Window* window, ElementData* element)
{
    ElementState* state = mapa_get(window->elements_states, &element->id);
    if (state)
    {
        state->layout = element->calculated_layout;
        update_element_state(window, state);
    }

    // our layout is done and we can calculate children
    _for_each_child(element)
    {
        child->calculated_layout = calculate_layout(child, element);
    }

    grow_children(window, element);

    _for_each_child(element)
    {
        finalize_element(window, child);
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
    ElementData* parent = &current_window->elements.items[current_window->current_element.index];
    current_window->current_element.id = id ? generate_element_id(id, current_window->current_element.id, parent->n_children) : id;

    vektor_add(current_window->elements, (ElementData){
        .parent_element = current_window->current_element.index,
        .id = current_window->current_element.id
    });
    current_window->current_element.index = current_window->elements.n_items - 1;
}

static RenderedLayout sum_layouts(RenderedLayout a, RenderedLayout b)
{
    i32 x = min(a.x, b.x);
    i32 y = min(a.y, b.y);
    i32 w = max(a.x + a.w, b.x + b.w) - x;
    i32 h = max(a.y + a.h, b.y + b.h) - y;
    return (RenderedLayout){ .x = x, .y = y, .w = w, .h = h };
}

void Ripple_submit_element(RippleElementConfig config)
{
    ElementData* element = &current_window->elements.items[current_window->current_element.index];
    ElementData* parent = &current_window->elements.items[element->parent_element];
    if (parent->n_children++ > 0)
    {
        ElementData* child = &current_window->elements.items[parent->last_child];
        child->next_sibling = current_window->current_element.index;
    }
    parent->last_child = current_window->current_element.index;

    // render_data is supposed to be set if render_data_size is also
    if (config.render_data_size)
        config.render_data = allocator_make_copy(current_window->config.allocator, config.render_data, config.render_data_size);

    element->config = config;

    //RippleElementLayoutConfig config = element->config.layout;
    //RenderedLayout layout = element->calculated_layout;
    element->children_bounds = element->calculated_layout = calculate_layout(element, parent);
    parent->children_bounds = sum_layouts(parent->children_bounds, element->calculated_layout);
}

void Ripple_pop_id(void)
{
    ElementData *element = &current_window->elements.items[current_window->current_element.index];
    ElementData *parent = &current_window->elements.items[element->parent_element];
    element->calculated_layout = calculate_layout(element, parent);
    current_window->current_element.index = element->parent_element;
    current_window->current_element.id = parent->id;
}

void Ripple_begin(Allocator* allocator)
{
}

void Ripple_end(void)
{

}

#endif // RIPPLE_IMPLEMENTATION


#ifdef RIPPLE_WIDGETS

static ElementState* _get_or_insert_current_element_state()
{
    ElementState* state = mapa_get(current_window->elements_states, &current_window->current_element.id);
    if (!state)
        state = mapa_insert(current_window->elements_states, &current_window->current_element.id, (ElementState){ 0 });

    state->frame_color = current_window->frame_color;
    return state;
}

static RippleElementState _get_current_element_state_state()
{
    ElementState* state = _get_or_insert_current_element_state();
    return state ? state->state : (RippleElementState){ 0 };
}

static RenderedLayout _get_current_element_rendered_layout()
{
    ElementState* state = _get_or_insert_current_element_state();
    return state ? state->layout : (RenderedLayout){ 0 };
}

#define STATE() (_get_current_element_state_state())

#define SHAPE() (_get_current_element_rendered_layout())

#define FOUNDATION ._type = SVT_RELATIVE_PARENT
#define REFINEMENT ._type = SVT_RELATIVE_CHILD
#define DEPTH(value, relation) { ._unsigned_value = (u32)((value) * (f32)(2<<14)), relation }
#define FIXED(value) { ._signed_value = value, ._type = SVT_FIXED }
#define GROW { ._type = SVT_GROW }

#define SURFACE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (Ripple_start_window((RippleWindowConfig) { __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; Ripple_finish_window(), LINE_UNIQUE_VAR(_rippleiter)++)

#define RIPPLE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (Ripple_push_id(LINE_UNIQUE_HASH), Ripple_submit_element((RippleElementConfig) { ._dummy = 0, __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; Ripple_pop_id(), LINE_UNIQUE_VAR(_rippleiter)++)

#define FORM(...) .layout = { __VA_ARGS__ }

#define OPEN_THE_VOID(allocator) Ripple_begin(allocator)
#define CLOSE_THE_VOID() Ripple_end()

#define CURSOR() current_window->cursor_state

typedef struct {
    RippleColor color;
} RippleRectangleConfig;

void render_rectangle(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleRectangleConfig rectangle_data = *(RippleRectangleConfig*)config.render_data;
    ripple_render_rect(layout.x, layout.y, layout.w, layout.h, rectangle_data.color);
}

#define RECTANGLE(...)\
  .render_func = render_rectangle,\
  .render_data = &(RippleRectangleConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleRectangleConfig)

typedef struct {
    RippleImage image;
} RippleImageConfig;

void render_image(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleImageConfig image_data = *(RippleImageConfig*)config.render_data;
    ripple_render_image(layout.x, layout.y, layout.w, layout.h, image_data.image);
}

#define IMAGE(...)\
  .render_func = render_image,\
  .render_data = &(RippleImageConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleImageConfig)

typedef struct {
    RippleColor color;
    const char* text;
    f32 font_size;
} RippleTextConfig;

void render_text(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleTextConfig text_data = *(RippleTextConfig*)config.render_data;
    ripple_render_text(layout.x, layout.y, text_data.text, text_data.font_size, text_data.color);
}

#define TEXT(...)\
  .render_func = render_text,\
  .render_data = &(RippleTextConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleTextConfig)

#define CENTERED_HORIZONTAL(...) do {\
        RIPPLE() {\
            RIPPLE();\
            { __VA_ARGS__ }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED_VERTICAL(...) do {\
        RIPPLE( FORM( .width = DEPTH(1.0f, REFINEMENT), .direction = cld_VERTICAL ) ) {\
            RIPPLE();\
            { __VA_ARGS__ }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED(...) CENTERED_HORIZONTAL(CENTERED_VERTICAL(__VA_ARGS__);)

#define RIPPLE_RENDER_BEGIN() Ripple_render_begin()
#define RIPPLE_RENDER_END(context) Ripple_render_end(context)

#endif // RIPPLE_WIDGETS

#endif // RIPPLE_H
