#ifndef RIPPLE_H
#define RIPPLE_H

// for offsetof
#include <stddef.h>

#include <printccy/printccy.h>
#include <marrow/marrow.h>
#include <marrow/vektor.h>
#include <marrow/allocator.h>
#include <marrow/mapa.h>

typedef enum {
    SVT_GROW = 0,
    SVT_PIXELS = 1,
    SVT_RELATIVE_CHILD = 2,
    SVT_RELATIVE_PARENT = 3,
} RippleSizingValueType;

#define RIPPLE_FLOAT_PRECISION 25

typedef enum {
    RCF_RGB = 0, // 0xrrggbb, alpha = 1.0f
    RCF_RGBA = 1 // rrggbbaa
} RippleColorFormat;

STRUCT(RippleColor) {
    u32 value;
    RippleColorFormat format;
};

STRUCT(MouseButtonState) {
    u8 pressed : 1;
    u8 released : 1;
    u8 held: 1;
};

STRUCT(RippleCursorState) {
    MouseButtonState left, right, middle;
    i32 x, y;
    i32 dx, dy;
    struct {
        bool consumed : 1;
        bool valid : 1;
    };
};

STRUCT(RippleSizingValue) {
    i32 _value : 30;
    RippleSizingValueType _type : 2;
};

STRUCT(RenderedLayout) {
    i32 x, y, w, h, max_w, min_w, max_h, min_h;
};

typedef enum {
    cld_VERTICAL = 0,
    cld_HORIZONTAL = 1,
} RippleChildLayoutDirection;

STRUCT(RippleElementLayoutConfig) {
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
        bool keep_inside : 1;
        RippleChildLayoutDirection direction : 1;
    };
};

struct RippleElementConfig;
struct RippleRenderData;
typedef void (render_func_t)(struct RippleElementConfig, RenderedLayout, void*, struct RippleRenderData);

STRUCT(RippleElementConfig) {
    RippleElementLayoutConfig layout;

    render_func_t* render_func;
    void* render_data;
    usize render_data_size;

    u8 layer;
};

STRUCT(RippleElementState) {
    struct {
        u8 _frame_color : 1;
        u8 clicked : 1; // left click pressed and hovered
        u8 released : 1; // left click released and hovered
        u8 hovered : 1; // as long as cursor is in bounds
        u8 is_held : 1; // if was clicked and until button is released regardless of hovered
        u8 is_weak_held : 1; // cancelled when loses hover
        u8 first_render : 1; // when called after not existing the previous frame
    };
};

STRUCT(ElementState) {
    RenderedLayout layout;
    RippleElementState state;
    union {
        u64 user_data;
        void* user_ptr;
    };
};

STRUCT(ElementData) {
    u64 id;
    RippleElementConfig config;
    RenderedLayout calculated_layout;

    u32 parent_element;
    u32 n_children;
    u32 last_child;
    u32 next_sibling;
    u32 prev_sibling; // equals parent index if first sibling

    bool update_state;
};

STRUCT(Window) {
    RippleCursorState cursor_state;
    RippleCursorState prev_cursor_state;
    VEKTOR(ElementData) elements;
    MAPA(u64, ElementState) elements_states;
    void* user_data;
    struct {
        u64 id;
        u32 index;
        ElementState* state;
    } current_element;
    u32 width, height;
    u32 current_layer;
};

STRUCT(RippleContext) {
    bool initialized;
    BumpAllocator frame_allocator;
    u32 frame_color;
    Window current_window;
};

#define RIPPLE_WGPU 1 << 0
#define RIPPLE_GLFW 1 << 1
#define RIPPLE_EMPTY 1 << 2

#ifndef RIPPLE_BACKEND
#define RIPPLE_BACKEND RIPPLE_WGPU | RIPPLE_GLFW
#endif // RIPPLE_BACKEND

#if (RIPPLE_BACKEND) & RIPPLE_WGPU
#include "backends/ripple_wgpu.h"
#endif
#if (RIPPLE_BACKEND) & RIPPLE_GLFW
#include "backends/ripple_glfw.h"
#endif
#if (RIPPLE_BACKEND) & RIPPLE_EMPTY
#include "backends/ripple_empty.h"
#endif

void ripple_push_id(u64);
void ripple_submit_element(RippleElementConfig config); // copies render data if its size is non zero
void ripple_pop_id(void);

RippleContext ripple_initialize(RippleBackendRendererConfig config);
void ripple_make_active_context(RippleContext* context);
void ripple_submit(RippleContext* context, u32 width, u32 height, RippleRenderData render_data);

#ifdef RIPPLE_IMPLEMENTATION
#undef RIPPLE_IMPLEMENTATION

thread_local RippleContext* _ripple_context = nullptr;

void ripple_reset(RippleContext* context)
{
    vektor_clear(context->current_window.elements);
    vektor_add(context->current_window.elements, (ElementData) { 0 });

    context->current_window.current_element.id = 0;
    context->current_window.current_element.index = 0;

    bump_allocator_reset(&context->frame_allocator);
}

RippleContext ripple_initialize(RippleBackendRendererConfig renderer_config)
{
    RippleContext context = { .initialized = true };
    context.frame_allocator = bump_allocator_create();
    ripple_backend_renderer_initialize(renderer_config);

    // TODO: use allocators here lol
    vektor_init(context.current_window.elements, 0, nullptr);
    mapa_init(context.current_window.elements_states, mapa_hash_u64, mapa_cmp_bytes, nullptr);

    ripple_reset(&context);

    return context;
}

void ripple_make_active_context(RippleContext* context)
{
    _ripple_context = context;
}

static void finalize_element(ElementData* element);
static void update_element_state(ElementState* state);
void ripple_submit(RippleContext* context, u32 width, u32 height, RippleRenderData render_data)
{
    Window* window = &context->current_window;

    window->width = width;
    window->height = height;

    RippleCursorState* state = &window->cursor_state;
    state->left.held   = (state->left.held   | state->left.pressed)   && !state->left.released;
    state->right.held  = (state->right.held  | state->right.pressed)  && !state->right.released;
    state->middle.held = (state->middle.held | state->middle.pressed) && !state->middle.released;
    state->consumed = false;

    if (window->prev_cursor_state.valid)
    {
        state->dx = state->x - window->prev_cursor_state.x;
        state->dy = window->prev_cursor_state.y - state->y;
    }

    window->prev_cursor_state = *state;

    window->elements.items[0].calculated_layout = (RenderedLayout){ .x = 0, .y = 0, .w = width, .h = height };

    finalize_element(&window->elements.items[0]);

    // remove dead elements
    for (i32 element_i = 0; element_i < (i64)window->elements_states.size; element_i++)
    {
        ElementState* state = mapa_get_at_index(window->elements_states, (u64)element_i);
        if(!state) continue;

        if (state->state._frame_color != context->frame_color)
        {
            mapa_remove_at_index(window->elements_states, (u64)element_i);
            element_i--;
            continue;
        }
    }
    context->frame_color = context->frame_color ? 0 : 1;

    u32 sorted[window->elements.n_items];
    sort_indices(sorted, window->elements.items, window->elements.n_items, a->config.layer < b->config.layer);

    // updating is done in reverse
    for (i32 i = array_len(sorted) - 1; i >= 0; i--)
    {
        ElementData* element = &window->elements.items[sorted[i]];
        if (!element->update_state) continue;
        ElementState* state = mapa_get(window->elements_states, &element->id);
        if (!state) continue;
        state->layout = element->calculated_layout;
        update_element_state(state);
    }

    state->left.pressed = false;
    state->right.pressed = false;
    state->middle.pressed = false;
    state->left.released = false;
    state->right.released = false;
    state->middle.released = false;

    ripple_backend_render_begin(width, height);

    // while rendering is done normally
    for (u32 i = 0; i < array_len(sorted); i++)
    {
        ElementData* element = &window->elements.items[sorted[i]];
        if (!element->config.render_func) continue;
        element->config.render_func(element->config, element->calculated_layout, window->user_data, render_data);
    }

    ripple_backend_render_end(render_data, (RippleColor){ 0 });

    ripple_reset(context);
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

static RenderedLayout element_calculate_children_bounds(ElementData* element)
{
    Window* window = &_ripple_context->current_window;
    RenderedLayout layout = { 0 };
    _for_each_child(element)
    {
        if (child->config.layout.fixed) continue;
        if (LAYOUT_POS(child->config.layout)._type == SVT_GROW)
            DIM(layout) += DIM(child->calculated_layout);
        if (LAYOUT_OTHER_POS(child->config.layout)._type == SVT_GROW)
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

static void element_position_children(ElementData* element)
{
    Window* window = &_ripple_context->current_window;
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

static void element_grow_children(ElementData* element)
{
    Window* window = &_ripple_context->current_window;
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

static void update_element_state(ElementState* state)
{
    Window* window = &_ripple_context->current_window;
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

static void finalize_element(ElementData* element)
{
    Window* window = &_ripple_context->current_window;
    _for_each_child(element)
    {
        element_apply_sizing(child, SVT_RELATIVE_PARENT, element->calculated_layout, (RenderedLayout){ 0 });

        child->calculated_layout.w = clamp(child->calculated_layout.w, child->calculated_layout.min_w, child->calculated_layout.max_w);
        child->calculated_layout.h = clamp(child->calculated_layout.h, child->calculated_layout.min_h, child->calculated_layout.max_h);

        if (element->config.layout.keep_inside)
        {
            element->calculated_layout.x = clamp(element->calculated_layout.x, 0, (i32)window->width - element->calculated_layout.w);
            element->calculated_layout.y = clamp(element->calculated_layout.y, 0, (i32)window->height - element->calculated_layout.h);
        }
    }

    element_grow_children(element);

    element_position_children(element);

    for (u32 i = element->last_child; i != 0; i = window->elements.items[i].prev_sibling)
    {
        finalize_element(&window->elements.items[i]);
    }
}

static u64 generate_element_id(u64 base, u64 parent_id, u32 index)
{
    return hash_u64(hash_combine(base, hash_u64(parent_id + (u64)index * 1000)));
}

// should be called before submit element
void ripple_push_id(u64 id)
{
    Window* window = &_ripple_context->current_window;
    ElementData* parent = &window->elements.items[window->current_element.index];
    window->current_element.id = id ? generate_element_id(id, window->current_element.id, parent->n_children) : id;

    vektor_add(window->elements, (ElementData){
        .parent_element = window->current_element.index,
        .id = window->current_element.id
    });
    window->current_element.index = window->elements.n_items - 1;
    window->current_element.state = nullptr;
}

void ripple_submit_element(RippleElementConfig config)
{
    Window* window = &_ripple_context->current_window;
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
        config.render_data = allocator_make_copy((Allocator*)&_ripple_context->frame_allocator, config.render_data, config.render_data_size, 1);

    element->config = config;
}

void ripple_pop_id(void)
{
    Window* window = &_ripple_context->current_window;
    ElementData *element = &window->elements.items[window->current_element.index];
    ElementData *parent = &window->elements.items[element->parent_element];

    element_apply_sizing(element, SVT_GROW, (RenderedLayout){ 0 }, (RenderedLayout){ 0 });
    element_apply_sizing(element, SVT_PIXELS, (RenderedLayout){ 0 }, (RenderedLayout){ 0 });
    RenderedLayout children = element_calculate_children_bounds(element);
    element_apply_sizing(element, SVT_RELATIVE_CHILD, (RenderedLayout){ 0 }, children);

    window->current_element.index = element->parent_element;
    window->current_element.id = parent->id;
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
static ElementState* _get_or_insert_current_element_state(void)
{
    Window* window = &_ripple_context->current_window;
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
    state->state._frame_color = _ripple_context->frame_color;
    return state;
}

// the underlying field is a u64 so dont expect too much
#define STATE_USER(type) *((type*)&_get_or_insert_current_element_state()->user_data)
#define STATE_PTR() (_get_or_insert_current_element_state()->user_ptr)
#define STATE() (_get_or_insert_current_element_state()->state)
#define SHAPE() (_get_or_insert_current_element_state()->layout)

#define RELATIVE(value, relation) { ._value = (i32)((value) * (f32)(2<<RIPPLE_FLOAT_PRECISION)), relation }
#define PIXELS(value) { ._value = value, ._type = SVT_PIXELS }
#define GROW { ._type = SVT_GROW }

#define RIPPLE(...) for (u8 LINE_UNIQUE_VAR(_rippleiter) = (ripple_push_id(LINE_UNIQUE_HASH), ripple_submit_element((RippleElementConfig) { .layer = _ripple_context->current_window.current_layer, __VA_ARGS__ }), 0); LINE_UNIQUE_VAR(_rippleiter) < 1; ripple_pop_id(),LINE_UNIQUE_VAR(_rippleiter)++)

#define FORM(...) .layout = { __VA_ARGS__ }

#define CURSOR() (_ripple_context->current_window.cursor_state)

#define RIPPLE_RAISE() for (u8 LINE_UNIQUE_VAR(_rippleiter) = (_ripple_context->current_window.current_layer++, 0); LINE_UNIQUE_VAR(_rippleiter) < 1; _ripple_context->current_window.current_layer--, LINE_UNIQUE_VAR(_rippleiter)++)

#define RIPPLE_RGB(v) (RippleColor){ .format = RCF_RGB, .value = v }
#define RIPPLE_RGBA(v) (RippleColor){ .format = RCF_RGBA, .value = v }

STRUCT(RippleRectangleConfig) {
    RippleColor color;
    RippleColor color1;
    RippleColor color2;
    RippleColor color3;
    RippleColor color4;
    f32 radius;
    f32 radiusBL;
    f32 radiusBR;
    f32 radiusTL;
    f32 radiusTR;
};

void render_rectangle(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleRectangleConfig rectangle_data = *(RippleRectangleConfig*)config.render_data;
    if (rectangle_data.color.value != 0 || rectangle_data.color.format != 0)
        rectangle_data.color1 = rectangle_data.color2 = rectangle_data.color3 = rectangle_data.color4 = rectangle_data.color;
    if (rectangle_data.radius != 0.0f)
        rectangle_data.radiusBL = rectangle_data.radiusBR = rectangle_data.radiusTL = rectangle_data.radiusTR = rectangle_data.radius;
    ripple_backend_render_rect(layout.x, layout.y, layout.w, layout.h,
                               rectangle_data.color1, rectangle_data.color2, rectangle_data.color3, rectangle_data.color4,
                               rectangle_data.radiusBL, rectangle_data.radiusBR, rectangle_data.radiusTL, rectangle_data.radiusTR
                               );
}

#define RECTANGLE(...)\
  .render_func = render_rectangle,\
  .render_data = &(RippleRectangleConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleRectangleConfig)

STRUCT(RippleImageConfig) {
    RippleImage image;
};

void render_image(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleImageConfig image_data = *(RippleImageConfig*)config.render_data;
    ripple_backend_render_image(layout.x, layout.y, layout.w, layout.h, image_data.image);
}

#define IMAGE(...)\
  .render_func = render_image,\
  .render_data = &(RippleImageConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleImageConfig)

STRUCT(RippleTextConfig) {
    RippleColor color;
    s8 text;
};

void render_text(RippleElementConfig config, RenderedLayout layout, void* window_user_data, RippleRenderData user_data)
{
    RippleTextConfig text_data = *(RippleTextConfig*)config.render_data;
    ripple_backend_render_text(layout.x, layout.y, text_data.text, layout.h, text_data.color);
}

#ifndef WORDS
#define WORDS(...)\
  .render_func = render_text,\
  .render_data = &(RippleTextConfig){__VA_ARGS__},\
  .render_data_size = sizeof(RippleTextConfig)
#endif // WORDS

#define CENTERED_HORIZONTAL(...) do {\
        RIPPLE( FORM( .direction = cld_HORIZONTAL )) {\
            RIPPLE();\
            { __VA_ARGS__; }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED_VERTICAL(...) do {\
        RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .direction = cld_VERTICAL ) ) {\
            RIPPLE();\
            { __VA_ARGS__; }\
            RIPPLE();\
        }\
} while (false)
#define CENTERED(...) CENTERED_HORIZONTAL(CENTERED_VERTICAL(__VA_ARGS__);)

#endif // RIPPLE_H
