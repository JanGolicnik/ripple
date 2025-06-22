#include <math.h>

#define RIPPLE_RENDERING_RAYLIB
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_raylib.h"

void full_test(f32 dt)
{
    static f32 time = 0.0f;
    time += dt;

    RIPPLE( FORM ( .direction = cld_VERTICAL ), RECTANGLE ( .color = 0xF8F8E1 ) )
    {
        RIPPLE( FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ) )
        {
            RIPPLE( FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                f32 t = (sinf(time) + 1.0f) * 0.5f;
                RIPPLE( FORM (.height = DEPTH(t * (3.0f/4.0f), FOUNDATION ) ) );

                RIPPLE( FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = STATE().hovered ? 0xFFD5EE : 0xFFC1DA ) );

                RIPPLE( RECTANGLE ( .color = STATE().hovered ? 0xFFA4CF : 0xFF90BB ) );
            }

            RIPPLE()
            {
                u32 w = SHAPE().w;
                u32 h = SHAPE().h;

                CENTERED(
                    f32 t_target = h ? 150.0f / (f32)h : 0.0f;
                    static f32 t = -1.0f; if (t < 0.0f) t = t_target;
                    RIPPLE( FORM( .width = FIXED((i32)((f32)w * t)), .height = FIXED((i32)((f32)h * t)), .min_width = FIXED(150), .min_height = FIXED(150) ), RECTANGLE ( .color = STATE().hovered ? 0x4D525A : 0x393E46 ) ){
                        if (STATE().is_held)
                            t_target = 1.0f;
                    }
                    t += (t_target - t) * (1.0f - expf(-dt * 20.0f));
                );
            }
        }

        RIPPLE( RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );

            RIPPLE()
            {
                RippleElementLayoutConfig small_guys_form = { .width = FIXED(4) };
                for(u32 i = 0; i < 50; i++)
                {
                    RIPPLE( .layout = small_guys_form );
                    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0xBAF9FF : 0x8ACCD5 ));
                }
                RIPPLE( .layout = small_guys_form );
            }

            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );
        }
    }
}

void number_test(f32 _)
{
    RIPPLE( RECTANGLE( .color = 0xffffff ) )
    {
        for (int i = 0; i < 200; i++)
        {
            RIPPLE( RECTANGLE( .color = 0x0 ) );
            RIPPLE();
        }
    }
}

void rgb_test(f32 _)
{
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0xff0000 : 0x000000 ) );
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0x00ff00 : 0x000000 ) );
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0x0000ff : 0x000000 ) );
}

void element_func_helper( u32 color, u32 hovered_color )
{
    RIPPLE( RECTANGLE( .color = STATE().hovered ? hovered_color : color ) );
}

void element_func_test(f32 _)
{
    element_func_helper(0xaa0000, 0xff0000);
    element_func_helper(0x00aa00, 0x00ff00);
    element_func_helper(0x0000aa, 0x0000ff);
}

void text_test(f32 _)
{
    RIPPLE( RECTANGLE( .color = 0xffffff ) ){
        i32 h = SHAPE().h;
        CENTERED_HORIZONTAL (
            const char* text = "Hello";
            f32 font_size = 128.0f;
            i32 text_w, text_h; ripple_measure_text(text, font_size, &text_w, &text_h);
            RIPPLE( RECTANGLE( .color = 0x0 ), FORM( .height = FIXED(h), .width = FIXED(text_w) ) )
            {
                CENTERED_VERTICAL
                (
                    RIPPLE( TEXT( .text = text, .font_size = font_size, .color = 0xffffff  ), FORM( .width = FIXED(text_w), .height = FIXED(text_h) ) );
                );
            }
        );
    }
}

Allocator widget_allocator;

void widget_slider(f32 width, f32 height, f32 *value, f32 max_value, f32 dt, f32 font_size)
{
    RIPPLE( FORM( .width = FIXED((u32)width), .height = FIXED((u32)height) ))
    {
        f32 slider_width = 10.0f;
        f32 text_width = height * 2.0f;

        // text
        RIPPLE( FORM( .width = FIXED( (u32)text_width ) ) )
        {
            f32 val = round(*value);
            u32 size = print(0, 0, "{}", (u32)val);
            u8* text = allocator_alloc(&widget_allocator, size + 1);
            print((void*)text, size, "{}", (u32)val);
            i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
            CENTERED(
                RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = (char*)text, .font_size = font_size));
            );
        }

        RIPPLE()
        {
            f32 target_value = !STATE().is_weak_held ? *value :
                clamp((f32)(CURSOR().x - SHAPE().x) / (f32)SHAPE().w, 0.0f, 1.0f) * max_value;

            CENTERED_VERTICAL(
                *value += (target_value - *value) * (1.0f - expf(-dt * 50.0f));
                f32 t = *value / max_value;
                f32 remaining_width = width - text_width - slider_width;
                RIPPLE( FORM( .width = FIXED(remaining_width * t), .height = DEPTH(0.5f, FOUNDATION) ), RECTANGLE( .color = 0xff0000 ));
            );

            RIPPLE( FORM( .width = FIXED((u32)slider_width) ), RECTANGLE( .color = 0x0 ) );
        }
    }
}

f32 values[] = { 0.0f, 20.0f, 40.0f, 60.0f, 80.0f, 100.0f };
u8 widget_arena[1024];
void widget_test(f32 dt)
{
    buf_set(widget_arena, 0, sizeof(widget_arena));
    LinearAllocatorContext linear_context = (LinearAllocatorContext){.data = widget_arena, sizeof(widget_arena)};
    widget_allocator = (Allocator){.context = (void*)&linear_context, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free };
    RIPPLE( FORM( .width = FIXED(300), .height = DEPTH(1.0f, REFINEMENT), .direction = cld_VERTICAL ) )
    {
        for (u32 i = 0; i < sizeof(values) / sizeof(values[0]); i++)
        {
            widget_slider(300.0f, 40.0f, &values[i], 100.0f, dt, 20.0f + 20.0f * 0.01f * values[i]);
        }
    }
}

typedef void (test_func)(f32);
test_func* test_funcs[] = { &full_test, &number_test, &rgb_test, &element_func_test, &text_test, &widget_test };
const char* test_func_labels[] = { "big", "n", "rgb", "func", "text", "widget" };
u32 light_colors_funcs[] = { 0xff0000, 0x00ff00, 0x0000ff, 0x00ffaa, 0xffffaa, 0xffaaff };
u32  dark_colors_funcs[] = { 0xaa0000, 0x00aa00, 0x0000aa, 0x00aa55, 0xaaaa55, 0xaa55aa };
int main(int argc, char* argv[])
{
    u32 test_index = 5;
    f32 accum_dt = 0.0f;
    u32 n_dt_samples = 0;
    while( SURFACE_IS_STABLE() )
    {
        f32 dt = GetFrameTime();

        n_dt_samples += 1;
        accum_dt += dt;
        if (accum_dt > 1.0f)
        {
            debug("Running at {} fps", 1.0 / (f64)(accum_dt / (f32)n_dt_samples));
            accum_dt = 0.0f;
            n_dt_samples = 0;
        }

        float font_size = 45.0f;

        SURFACE( .title = "surface", .width = 800, .height = 800 + 45 )
        {
            RIPPLE( FORM( .direction = cld_VERTICAL ) )
            {
                RIPPLE( FORM( .height = FIXED(font_size) ) )
                {
                    for (u32 i = 0; i < sizeof(test_funcs) / sizeof(test_funcs[0]); i++)
                    {
                        u32 color = light_colors_funcs[i];
                        u32 dark_color = dark_colors_funcs[i];
                        i32 text_w, text_h; ripple_measure_text((const char*)test_func_labels[i], font_size, &text_w, &text_h);
                        RIPPLE( FORM( .width = FIXED(text_w) ),
                                TEXT( .color = (STATE().hovered || test_index == i) ? color : dark_color,
                                      .font_size = font_size,
                                      .text = test_func_labels[i] )
                        ) {
                            if (STATE().clicked) test_index = i;
                        }
                        RIPPLE(FORM(.width = FIXED(20)));
                    }
                }

                RIPPLE( )
                {
                    test_funcs[test_index](dt);
                }
            }
        }
    }

    return 0;
}
