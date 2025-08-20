#ifndef RIPPLE_EMPTY_H
#define RIPPLE_EMPTY_H

#include <marrow/marrow.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_EMPTY_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

typedef u32 RippleBackendConfig;

typedef u32 RippleRenderData;

typedef u32 RippleImage;

#include "../ripple.h"

#ifdef RIPPLE_EMPTY_IMPLEMENTATION


RippleBackendConfig ripple_get_default_backend_config()
{
    return (RippleBackendConfig){ 0 };
}

void ripple_backend_initialize(RippleBackendConfig config)
{
}

void ripple_window_begin(u64 id, RippleWindowConfig config)
{
}

void ripple_window_end()
{
}

void ripple_window_close(u64 id)
{
}

void ripple_get_window_size(u32* width, u32* height)
{
    *width = 100; *height = 100;
}

RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config)
{
    return (RippleWindowState){ .is_open = true };
}

RippleCursorState ripple_update_cursor_state(RippleCursorState state)
{
    return (RippleCursorState){ 0 };
}

RippleRenderData ripple_render_begin()
{
    return (RippleRenderData){ 0 };
}

void ripple_render_window_begin(u64 id, RippleRenderData render_data)
{
}

void ripple_render_window_end(RippleRenderData render_data)
{
}

void ripple_render_end(RippleRenderData render_data)
{
}

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, RippleColor color)
{
}

void ripple_render_image(i32 x, i32 y, i32 w, i32 h, RippleImage image)
{
}

void ripple_measure_text(s8 text, f32 font_size, i32* out_w, i32* out_h)
{
}

void ripple_render_text(i32 pos_x, i32 pos_y, s8 text, f32 font_size, RippleColor color)
{
}

#endif // RIPPLE_EMPTY_IMPLEMENTATION

#endif // RIPPLE_EMPTY_H
