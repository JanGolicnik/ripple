#ifndef RIPPLE_EMPTY_H
#define RIPPLE_EMPTY_H

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_EMPTY_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

STRUCT(RippleBackendRendererConfig) {
    u32 _empty;
};

STRUCT(RippleRenderData)  {
    u32 _empty;
};

typedef u32 RippleImage;

#ifdef RIPPLE_EMPTY_IMPLEMENTATION

void ripple_backend_renderer_initialize(RippleBackendRendererConfig config) { }

void ripple_backend_render_begin(u32 width, u32 height) { }

void ripple_backend_render_end(RippleRenderData render_data, RippleColor clear_color) { }

void ripple_backend_render_rect(i32 x, i32 y, i32 w, i32 h, RippleColor color1, RippleColor color2, RippleColor color3, RippleColor color4, f32 radius1, f32 radius2, f32 radius3, f32 radius4) { }

void ripple_backend_render_image(i32 x, i32 y, i32 w, i32 h, RippleImage image) { }

void ripple_measure_text(str text, f32 font_size, i32* out_w, i32* out_h)
{
    *out_w = str_len(text) * font_size;
    *out_h = font_size;
}

void ripple_backend_render_text(i32 pos_x, i32 pos_y, str text, f32 font_size, RippleColor color) { }

#endif // RIPPLE_WGPU_IMPLEMENTATION

#endif // RIPPLE_WGPU_H
