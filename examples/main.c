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

typedef void (test_func)(f32);
test_func* test_funcs[] = { &full_test, &number_test, &rgb_test, &element_func_test, &text_test };

int main(int argc, char* argv[])
{
    u32 test_index = 4;
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

        SURFACE( .title = "surface", .width = 800, .height = 800 + 45 )
        {
            RIPPLE( FORM( .direction = cld_VERTICAL ) )
            {
                RIPPLE( FORM( .height = FIXED(45) ) )
                {
                    u32 i = 0;
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = (STATE().hovered || test_index == i++) ? 0xff0000 : 0xaa0000 ) ) { if (STATE().clicked) test_index = i; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = (STATE().hovered || test_index == i++) ? 0x00ff00 : 0x00aa00 ) ) { if (STATE().clicked) test_index = i; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = (STATE().hovered || test_index == i++) ? 0x0000ff : 0x0000aa ) ) { if (STATE().clicked) test_index = i; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = (STATE().hovered || test_index == i++) ? 0x00ffaa : 0x00aa55 ) ) { if (STATE().clicked) test_index = i; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = (STATE().hovered || test_index == i++) ? 0xffffaa : 0xaaaa55 ) ) { if (STATE().clicked) test_index = i; }
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
