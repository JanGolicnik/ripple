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

typedef enum {
    RCF_RGB = 0, // 0xrrggbb, alpha = 1.0f
    RCF_RGBA = 1 // rrggbbaa
} RippleColorFormat;

typedef struct {
    u32 value;
    RippleColorFormat format;
} RippleColor;

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
    bool consumed;
} RippleCursorState;

typedef struct {
    s8 title;
    bool* is_open;
    Allocator* allocator;
    Allocator* frame_allocator;

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

#define RIPPLE_WGPU 1 << 0
#define RIPPLE_GLFW 1 << 1
#define RIPPLE_SDL  1 << 2

#ifndef RIPPLE_BACKEND
#define RIPPLE_BACKEND RIPPLE_WGPU | RIPPLE_GLFW
#endif // RIPPLE_BACKEND

struct RippleBackendWindow;
struct RippleBackendWindow* _ripple_get_window_impl(u64 id);

#if (RIPPLE_BACKEND) & RIPPLE_GLFW
#include "backends/ripple_glfw.h"
#endif
#if (RIPPLE_BACKEND) & RIPPLE_SDL
#include "backends/ripple_sdl.h"
#endif
#if (RIPPLE_BACKEND) & RIPPLE_WGPU
#include "backends/ripple_wgpu.h"
#endif

typedef enum {
    SVT_GROW = 0,
    SVT_PIXELS = 1,
    SVT_RELATIVE_CHILD = 2,
    SVT_RELATIVE_PARENT = 3,
} RippleSizingValueType;

#define RIPPLE_FLOAT_PRECISION 25

typedef struct {
    i32 _value : 30;
    RippleSizingValueType _type : 2;
} RippleSizingValue;

typedef struct {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
    i32 max_w;
    i32 min_w;
    i32 max_h;
    i32 min_h;
} RenderedLayout;

typedef enum {
    cld_VERTICAL = 0,
    cld_HORIZONTAL = 1,
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
typedef void (render_func_t)(struct RippleBackendWindowRenderer*, struct RippleElementConfig, RenderedLayout, void*, RippleRenderData);

typedef struct RippleElementConfig {
    RippleElementLayoutConfig layout;

    render_func_t* render_func;
    void* render_data;
    usize render_data_size;

    struct{
        bool _dummy : 1;
    };
} RippleElementConfig;

typedef struct {
    struct {
        u8 _frame_color : 1;
        u8 clicked : 1; // left click pressed and hovered
        u8 released : 1; // left click released and hovered
        u8 hovered : 1; // as long as cursor is in bounds
        u8 is_held : 1; // if was clicked and until button is released regardless of hovered
        u8 is_weak_held : 1; // cancelled when loses hover
        u8 first_render : 1; // when called after not existing the previous frame
    };
} RippleElementState;

struct Window;

void ripple_start_window(RippleWindowConfig config);
void ripple_finish_window(void);

void ripple_push_id(struct Window* window, u64);
void ripple_submit_element(struct Window* window, RippleElementConfig config); // copies render data if its size is non zero
void ripple_pop_id(struct Window* window);

void ripple_begin(Allocator* allocator);
void ripple_end(void);

RippleRenderData ripple_render_begin();
void ripple_render_end(RippleRenderData user_data);

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

typedef struct {
    RenderedLayout layout;
    RippleElementState state;
    union {
        u64 user_data;
        void* user_ptr;
    };
} ElementState;

typedef struct {
    u64 id;
    RippleElementConfig config;
    RenderedLayout calculated_layout;

    u32 parent_element;
    u32 n_children;
    u32 last_child;
    u32 next_sibling;
    u32 prev_sibling; // equals parent index if first sibling

    bool update_state;
} ElementData;

typedef struct Window {
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
        ElementState* state;
    } current_element;

    void* user_data;

    RippleBackendWindow window_impl;
    RippleBackendWindowRenderer window_renderer_impl;
} Window;

thread_local struct {
    BumpAllocator frame_allocator;
    bool initialized;

    MAPA(u64, Window) windows;
    Window* current_window;
    u32 current_frame_color;
} _ripple_context;

void ripple_initialize(RippleBackendWindowConfig window_config, RippleBackendRendererConfig renderer_config)
{
    _ripple_context.frame_allocator = bump_allocator_create();

    // this should use an allocator
    mapa_init(_ripple_context.windows, mapa_hash_u64, mapa_cmp_bytes, nullptr);

    ripple_backend_window_initialize(window_config);
    ripple_backend_renderer_initialize(renderer_config);

    _ripple_context.initialized = true;
}

void ripple_start_window(RippleWindowConfig config)
{
    if (!_ripple_context.initialized)
    {
        ripple_initialize(ripple_backend_window_default_config(), ripple_backend_renderer_default_config());
    }

    if (!config.frame_allocator)
    {
        config.frame_allocator = (Allocator*)&_ripple_context.frame_allocator;
    }

    u64 parent_id = _ripple_context.current_window ? _ripple_context.current_window->id : 0;

    u64 window_id = hash_buf(config.title);
    Window* window = _ripple_context.current_window = mapa_get(_ripple_context.windows, &window_id);
    if (!window)
    {
        RippleBackendWindow window_impl = ripple_backend_window_create(window_id, config);
        RippleBackendWindowRenderer window_renderer_impl = ripple_backend_window_renderer_create(window_id, config, &window_impl);
        window = _ripple_context.current_window =
            mapa_insert(_ripple_context.windows, &window_id, ((Window){ .id = window_id, .window_impl = window_impl, .window_renderer_impl = window_renderer_impl }));
    }

    window->parent_id = parent_id;
    window->config = config;

    // update backend and state
    {
        window->cursor_state.left.held |= window->cursor_state.left.pressed;
        window->cursor_state.right.held |= window->cursor_state.right.pressed;
        window->cursor_state.middle.held |= window->cursor_state.middle.pressed;

        window->cursor_state.consumed = false;

        ripple_backend_window_update(&window->window_impl, &window->config, &window->state, &window->cursor_state);
        if (window->window_impl.resized)
        {
            debug("width: {}", window->config.width);
        }
        if (config.is_open)
            *config.is_open = window->state.is_open;

        if (window->cursor_state.left.released) window->cursor_state.left.held = 0;
        if (window->cursor_state.right.released) window->cursor_state.right.held = 0;
        if (window->cursor_state.middle.released) window->cursor_state.middle.held = 0;
    }

    // reset window elements
    {
        if (window->elements.items) vektor_clear(window->elements);
        else vektor_init(window->elements, 0, config.allocator);

        if (!window->elements_states.entries)
            mapa_init(window->elements_states, mapa_hash_u64, mapa_cmp_bytes, config.allocator);

        vektor_add(window->elements, (ElementData) {
                .calculated_layout = {
                    .x = 0,
                    .y = 0,
                    .w = window->config.width,
                    .h = window->config.height
                },
        });
        window->current_element.id = 0;
        window->current_element.index = 0;

        window->frame_color = _ripple_context.current_frame_color;
    }
}

struct RippleBackendWindow* _ripple_get_window_impl(u64 id)
{
    return &mapa_get(_ripple_context.windows, &id)->window_impl;
}

void ripple_finish_window()
{
    _ripple_context.current_window = _ripple_context.current_window->parent_id ?
        mapa_get(_ripple_context.windows, &_ripple_context.current_window->parent_id) :
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

        if (state->state._frame_color != window->frame_color)
        {
            mapa_remove_at_index(window->elements_states, (u64)element_i);
            element_i--;
            continue;
        }
    }
}

RippleRenderData ripple_render_begin()
{
    for (i32 window_i = 0; window_i < (i64)_ripple_context.windows.size; window_i++)
    {
        Window* window = mapa_get_at_index(_ripple_context.windows, (u64)window_i);
        if(!window) continue;

        if (window->frame_color != _ripple_context.current_frame_color)
        {
            ripple_backend_window_close(&window->window_impl);
            vektor_free(window->elements);
            mapa_free(window->elements_states);

            mapa_remove_at_index(_ripple_context.windows, (u64)window_i);
            window_i--;
            continue;
        }

        update_window(window);
    }

    _ripple_context.current_frame_color = _ripple_context.current_frame_color ? 0 : 1;

    return ripple_backend_render_begin();
}

void ripple_render_end(RippleRenderData render_data)
{
    for (u32 window_i = 0; window_i < _ripple_context.windows.size; window_i++)
    {
        Window* window = mapa_get_at_index(_ripple_context.windows, window_i);
        if (!window) continue;

        ripple_backend_render_window_begin(&window->window_impl, &window->window_renderer_impl, render_data);
        if (window->elements.n_items){
            render_element(window, &window->elements.items[0], window->user_data, render_data);
        }
        ripple_backend_render_window_end(&window->window_renderer_impl, render_data);
    }

    bump_allocator_reset(&_ripple_context.frame_allocator);

    ripple_backend_render_end(render_data);

    for (u32 window_i = 0; window_i < _ripple_context.windows.size; window_i++)
    {
        Window* window = mapa_get_at_index(_ripple_context.windows, window_i);
        if (!window) continue;
        ripple_backend_window_present(&window->window_renderer_impl);
    }
}

#define _I1 LINE_UNIQUE_VAR(_i)
#define _for_each_child(el) u32 _I1 = 0; for (ElementData* child = el + 1; _I1 < el->n_children; child = &window->elements.items[child->next_sibling], _I1++ )

#define APPLY_SIZING(var, value, grow, parent, child)\
if (value._type == type)\
    switch(type){\
        case SVT_GROW: var = grow; break;\
        case SVT_PIXELS: var = value._value; break;\
        case SVT_RELATIVE_CHILD: case SVT_RELATIVE_PARENT:\
            var = (type == SVT_RELATIVE_PARENT ? parent : child) * ((f32)value._value / (f32)(2<<RIPPLE_FLOAT_PRECISION)); break;\
    }

#define DIM(arg) (*(element->config.layout.direction ? &(arg).w : &(arg).h))
#define OTHER_DIM(arg) (*(element->config.layout.direction ? &(arg).h : &(arg).w))
#define MAX_DIM(arg) (*(element->config.layout.direction ? &(arg).max_w : &(arg).max_h))
#define POS(arg) (*(element->config.layout.direction ? &(arg).x : &(arg).y))
#define OTHER_POS(arg) (*(element->config.layout.direction ? &(arg).y : &(arg).x))
#define LAYOUT_DIM(arg) (*(element->config.layout.direction ? &(arg).width : &(arg).height))
#define LAYOUT_OTHER_DIM(arg) (*(element->config.layout.direction ? &(arg).height : &(arg).width))
#define LAYOUT_POS(arg) (*(element->config.layout.direction ? &(arg).x : &(arg).y))
#define LAYOUT_OTHER_POS(arg) (*(element->config.layout.direction ? &(arg).y : &(arg).x))

static RenderedLayout element_calculate_children_bounds(Window* window, ElementData* element)
{
    RenderedLayout layout = { 0 };
    _for_each_child(element)
    {
        if (child->config.layout.fixed) continue;
        DIM(layout) += DIM(child->calculated_layout);
        OTHER_DIM(layout) = max(OTHER_DIM(layout), OTHER_DIM(child->calculated_layout));
    }
    return layout;
}

static void element_apply_sizing(ElementData* element, RippleSizingValueType type, RenderedLayout parent, RenderedLayout children)
{
    RippleElementLayoutConfig config = element->config.layout;
    RenderedLayout layout = element->calculated_layout;

    APPLY_SIZING(layout.w, config.width, 0, parent.w, children.w);
    APPLY_SIZING(layout.max_w, config.max_width, I32_MAX, parent.w, children.w);
    APPLY_SIZING(layout.min_w, config.min_width, 0, parent.w, children.w);

    APPLY_SIZING(layout.h, config.height, 0, parent.h, children.h);
    APPLY_SIZING(layout.max_h, config.max_height, I32_MAX, parent.h, children.h);
    APPLY_SIZING(layout.min_h, config.min_height, 0, parent.h, children.h);

    APPLY_SIZING(layout.x, config.x, 0, parent.w, children.w);
    APPLY_SIZING(layout.y, config.y, 0, parent.h, children.h);

    element->calculated_layout = layout;
}

static void element_position_children(Window* window, ElementData* element)
{
    u32 offset = 0;
    _for_each_child(element) {
        if (child->config.layout.fixed) continue;
        if (LAYOUT_POS(child->config.layout)._type != SVT_GROW)
        {
            POS(child->calculated_layout) += POS(element->calculated_layout);
        }
        else
        {
            POS(child->calculated_layout) = POS(element->calculated_layout) + offset;
            offset += DIM(child->calculated_layout);
        }

        if (LAYOUT_OTHER_POS(child->config.layout)._type != SVT_GROW)
        {
            OTHER_POS(child->calculated_layout) += OTHER_POS(element->calculated_layout);
        }
        else
        {
            OTHER_POS(child->calculated_layout) = OTHER_POS(element->calculated_layout);
        }
    }
}

static void element_grow_children(Window* window, ElementData* element)
{
    if (element->n_children == 0)
        return;

    u32 free_space = (u32)DIM(element->calculated_layout);
    _for_each_child(element) {
        if (child->config.layout.fixed) continue;
        free_space = free_space >= (u32)DIM(child->calculated_layout) ? free_space - (u32)DIM(child->calculated_layout) : 0;
    }

    while(free_space > 0)
    {
        #define doest_grow_or_fixed_or_max_width (LAYOUT_DIM(child->config.layout))._type != SVT_GROW || child->config.layout.fixed || DIM(child->calculated_layout) >= MAX_DIM(child->calculated_layout)

        u32 smallest_width = free_space;
        u32 second_smallest_width = free_space; // others are then resized to this
        _for_each_child(element) {
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
        _for_each_child(element) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            n_smallest += 1;
        }

        if (n_smallest == 0)
            break;

        u32 size_diff = second_smallest_width - smallest_width;
        u32 amount_to_add = min(size_diff, free_space / n_smallest);
        _for_each_child(element) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            DIM(child->calculated_layout) += amount_to_add;
        }

        smallest_width += amount_to_add;

        // distribute the remainder
        u32 remaining_after_rounding = min(size_diff * n_smallest, free_space) - amount_to_add * n_smallest;
        _for_each_child(element) {
            if (doest_grow_or_fixed_or_max_width || (u32)DIM(child->calculated_layout) != smallest_width) continue;
            if (remaining_after_rounding-- == 0) break;
            child->calculated_layout.w += 1;
        }

        free_space -= second_smallest_width;
        #undef doest_grow_or_fixed_or_max_width
    }

    _for_each_child(element) {
        if (child->config.layout.fixed || LAYOUT_OTHER_DIM(child->config.layout)._type != SVT_GROW) continue;
        OTHER_DIM(child->calculated_layout) = OTHER_DIM(element->calculated_layout);
    }

}

static void update_element_state(Window* window, ElementState* state)
{
    state->state.hovered = !window->cursor_state.consumed && (
        window->cursor_state.x >= state->layout.x && window->cursor_state.x < state->layout.x + state->layout.w &&
        window->cursor_state.y >= state->layout.y && window->cursor_state.y < state->layout.y + state->layout.h
    );

    state->state.clicked = state->state.hovered && window->cursor_state.left.pressed;
    state->state.released = state->state.hovered && window->cursor_state.left.released;

    if (state->state.hovered)
    {
        window->cursor_state.consumed = true;
    }

    state->state.is_held      = (window->cursor_state.left.pressed && state->state.clicked) ||
                                (window->cursor_state.left.held && state->state.is_held);
    state->state.is_weak_held = state->state.hovered &&
                                ((window->cursor_state.left.pressed && state->state.clicked) ||
                                 (window->cursor_state.left.held && state->state.is_held));
}

static void finalize_element(Window* window, ElementData* element)
{
    _for_each_child(element)
    {
        element_apply_sizing(child, SVT_RELATIVE_PARENT, element->calculated_layout, (RenderedLayout){ 0 });

        child->calculated_layout.w = clamp(child->calculated_layout.w, child->calculated_layout.min_w, child->calculated_layout.max_w);
        child->calculated_layout.h = clamp(child->calculated_layout.h, child->calculated_layout.min_h, child->calculated_layout.max_h);
    }

    element_grow_children(window, element);

    element_position_children(window, element);

    for (u32 i = element->last_child; i != 0; i = window->elements.items[i].prev_sibling)
    {
        finalize_element(window, &window->elements.items[i]);
    }

    if (element->update_state)
    {
        ElementState* state = mapa_get(window->elements_states, &element->id);
        if (state)
        {
            state->layout = element->calculated_layout;
            update_element_state(window, state);
        }
    }
}

// recursively renderes elements
static void render_element(Window* window, ElementData* element, void* window_user_data, RippleRenderData render_data)
{
    if (element->config.render_func)
        element->config.render_func(&window->window_renderer_impl, element->config, element->calculated_layout, window_user_data, render_data);

    allocator_free(window->config.frame_allocator, element->config.render_data, element->config.render_data_size);

    _for_each_child(element)
    {
        render_element(window, child, window_user_data, render_data);
    }
}

static u64 generate_element_id(u64 base, u64 parent_id, u32 index)
{
    return hash_u64(hash_combine(base, hash_u64(parent_id + (u64)index * 1000)));
}

// should be called before submit element
void ripple_push_id(Window* window, u64 id)
{
    ElementData* parent = &window->elements.items[window->current_element.index];
    window->current_element.id = id ? generate_element_id(id, window->current_element.id, parent->n_children) : id;

    vektor_add(window->elements, (ElementData){
        .parent_element = window->current_element.index,
        .id = window->current_element.id
    });
    window->current_element.index = window->elements.n_items - 1;
}

void ripple_submit_element(Window* window, RippleElementConfig config)
{
    ElementData* element = &window->elements.items[window->current_element.index];
    ElementData* parent = &window->elements.items[element->parent_element];
    if (parent->n_children++ > 0)
    {
        ElementData* child = &window->elements.items[parent->last_child];
        child->next_sibling = window->current_element.index;
    }
    element->prev_sibling = parent->last_child;
    parent->last_child = window->current_element.index;

    // render_data is supposed to be set if render_data_size is also
    if (config.render_data_size)
        config.render_data = allocator_make_copy(window->config.frame_allocator, config.render_data, config.render_data_size, 1);

    element->config = config;
}

void ripple_pop_id(Window* window)
{
    ElementData *element = &window->elements.items[window->current_element.index];
    ElementData *parent = &window->elements.items[element->parent_element];

    element_apply_sizing(element, SVT_GROW, (RenderedLayout){ 0 }, (RenderedLayout){ 0 });
    element_apply_sizing(element, SVT_PIXELS, (RenderedLayout){ 0 }, (RenderedLayout){ 0 });
    RenderedLayout children = element_calculate_children_bounds(window, element);
    element_apply_sizing(element, SVT_RELATIVE_CHILD, (RenderedLayout){ 0 }, children);

    window->current_element.index = element->parent_element;
    window->current_element.id = parent->id;
    window->current_element.state = nullptr;
}

void ripple_begin(Allocator* allocator)
{
}

void ripple_end(void)
{

}

#undef DIM
#undef MAX_DIM
#undef POS
#undef OTHER_POS
#undef LAYOUT_DIM
#undef LAYOUT_MAX_DIM
#undef LAYOUT_MIN_DIM

#undef APPLY_SIZING

#undef _for_each_child

#endif // RIPPLE_IMPLEMENTATION

// TODO: cache this somehow
static ElementState* _get_or_insert_current_element_state(Window* window)
{
    if (window->current_element.state)
        return window->current_element.state;

    ElementState* state = mapa_get(window->elements_states, &window->current_element.id);
    if (!state)
    {
        state = mapa_insert(window->elements_states, &window->current_element.id, (ElementState){ 0 });
        state->state.first_render = true;
    }
    else
    {
        state->state.first_render = false;
    }

    window->elements.items[window->current_element.index].update_state = true;
    window->current_element.state = state;
    state->state._frame_color = window->frame_color;
    return state;
}

// the underlying field is a u64 so dont expect too much
#define STATE_USER(type) *((type*)&_get_or_insert_current_element_state(_ripple_context.current_window)->user_data)
#define STATE_PTR() (_get_or_insert_current_element_state(_ripple_context.current_window)->user_ptr)
#define STATE() (_get_or_insert_current_element_state(_ripple_context.current_window)->state)
#define SHAPE() (_get_or_insert_current_element_state(_ripple_context.current_window)->layout)

#define RELATIVE(value, relation) { ._value = (i32)((value) * (f32)(2<<RIPPLE_FLOAT_PRECISION)), relation }
#define PIXELS(value) { ._value = value, ._type = SVT_PIXELS }
#define GROW { ._type = SVT_GROW }

#define SURFACE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (ripple_start_window((RippleWindowConfig) { __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; ripple_finish_window(), LINE_UNIQUE_VAR(_rippleiter)++)

#define RIPPLE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (ripple_push_id(_ripple_context.current_window, LINE_UNIQUE_HASH), ripple_submit_element(_ripple_context.current_window, (RippleElementConfig) { ._dummy = 0, __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; ripple_pop_id(_ripple_context.current_window), LINE_UNIQUE_VAR(_rippleiter)++)

#define FORM(...) .layout = { __VA_ARGS__ }

#define OPEN_THE_VOID(allocator) ripple_begin(allocator)
#define CLOSE_THE_VOID() ripple_end()

#define CURSOR() (_ripple_context.current_window->cursor_state)

#define RIPPLE_RGB(v) (RippleColor){ .format = RCF_RGB, .value = v }
#define RIPPLE_RGBA(v) (RippleColor){ .format = RCF_RGBA, .value = v }

typedef struct {
    RippleColor color;
    RippleColor color1;
    RippleColor color2;
    RippleColor color3;
    RippleColor color4;
} RippleRectangleConfig;

void render_rectangle(RippleBackendWindowRenderer* renderer, RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleRectangleConfig rectangle_data = *(RippleRectangleConfig*)config.render_data;
    if (rectangle_data.color.value != 0 || rectangle_data.color.format != 0)
    {
        rectangle_data.color1 = rectangle_data.color2 = rectangle_data.color3 = rectangle_data.color4 = rectangle_data.color;
    }
    ripple_backend_render_rect(renderer, layout.x, layout.y, layout.w, layout.h, rectangle_data.color1, rectangle_data.color2, rectangle_data.color3, rectangle_data.color4);
}

#define RECTANGLE(...)\
  .render_func = render_rectangle,\
  .render_data = &(RippleRectangleConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleRectangleConfig)

typedef struct {
    RippleImage image;
} RippleImageConfig;

void render_image(RippleBackendWindowRenderer* renderer, RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleImageConfig image_data = *(RippleImageConfig*)config.render_data;
    ripple_backend_render_image(renderer, layout.x, layout.y, layout.w, layout.h, image_data.image);
}

#define IMAGE(...)\
  .render_func = render_image,\
  .render_data = &(RippleImageConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleImageConfig)

typedef struct {
    RippleColor color;
    s8 text;
} RippleTextConfig;

void render_text(RippleBackendWindowRenderer* renderer, RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleTextConfig text_data = *(RippleTextConfig*)config.render_data;
    ripple_backend_render_text(renderer, layout.x, layout.y, text_data.text, layout.h, text_data.color);
}

#ifndef WORDS
#define WORDS(...)\
  .render_func = render_text,\
  .render_data = &(RippleTextConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleTextConfig)
#endif // WORDS

#define CENTERED_HORIZONTAL(...) do {\
        RIPPLE() {\
            RIPPLE();\
            { __VA_ARGS__ }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED_VERTICAL(...) do {\
        RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .direction = cld_VERTICAL ) ) {\
            RIPPLE();\
            { __VA_ARGS__ }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED(...) CENTERED_HORIZONTAL(CENTERED_VERTICAL(__VA_ARGS__);)

#define RIPPLE_RENDER_BEGIN() ripple_render_begin()
#define RIPPLE_RENDER_END(_ripple_context) ripple_render_end(_ripple_context)

#endif // RIPPLE_H
