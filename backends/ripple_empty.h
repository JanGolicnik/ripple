#ifndef RIPPLE_EMPTY_H
#define RIPPLE_EMPTY_H

#include <marrow/marrow.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_EMPTY_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

/* typedef void* RippleBackendWindowConfig; */
/* typedef struct RippleBackendWindow { void* i; } RippleBackendWindow; */
typedef void* RippleBackendRendererConfig;
typedef struct RippleBackendWindowRenderer { void* i; } RippleBackendWindowRenderer;
typedef void* RippleRenderData;
typedef void* RippleImage;

#include "../ripple.h"

#ifdef RIPPLE_EMPTY_IMPLEMENTATION

/* RippleBackendWindowConfig ripple_backend_window_default_config() */
/* { */
/*     return nullptr; */
/* } */

/* void ripple_backend_window_initialize(RippleBackendWindowConfig config) */
/* { */
/* } */

/* RippleBackendWindow ripple_backend_window_create(u64 id, RippleWindowConfig config) */
/* { */
/*     return (RippleBackendWindow) { 0 }; */
/* } */

/* void ripple_backend_window_update(RippleBackendWindow* window, RippleWindowConfig* config, RippleWindowState* window_state, RippleCursorState* cursor_state) */
/* { */
/*     window_state->is_open = true; */
/* } */

/* void ripple_backend_window_close(RippleBackendWindow* window) */
/* { */
/* } */

RippleBackendRendererConfig ripple_backend_renderer_default_config()
{
    return nullptr;
}

void ripple_backend_renderer_initialize(RippleBackendRendererConfig config)
{
}

RippleBackendWindowRenderer ripple_backend_window_renderer_create(u64 id, RippleWindowConfig config, const RippleBackendWindow* window)
{
    return (RippleBackendWindowRenderer) { 0 };
}

RippleRenderData ripple_backend_render_begin()
{
    return nullptr;
}

void ripple_backend_render_window_begin(RippleBackendWindow* window, RippleBackendWindowRenderer* renderer, RippleRenderData render_data)
{
}

void ripple_backend_render_window_end(RippleBackendWindowRenderer* window, RippleRenderData render_data, RippleColor clear_color)
{
}

void ripple_backend_render_end(RippleRenderData render_data)
{
}

void ripple_backend_window_present(RippleBackendWindowRenderer* renderer)
{
}

void ripple_backend_render_rect(RippleBackendWindowRenderer* window, i32 x, i32 y, i32 w, i32 h, RippleColor color1, RippleColor color2, RippleColor color3, RippleColor color4)
{
}

void ripple_backend_render_image(RippleBackendWindowRenderer* window, i32 x, i32 y, i32 w, i32 h, RippleImage image)
{
}

void ripple_measure_text(s8 text, f32 font_size, i32* out_w, i32* out_h)
{
}

void ripple_backend_render_text(RippleBackendWindowRenderer* window, i32 pos_x, i32 pos_y, s8 text, f32 font_size, RippleColor color)
{
}

#endif // RIPPLE_EMPTY_IMPLEMENTATION

#endif // RIPPLE_EMPTY_H
