#define debug(...) (void)0

#define RIPPLE_RENDERING_RAYLIB
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "ripple.h"

void full_test(void)
{
    RIPPLE( FORM ( .direction = cld_VERTICAL ), RECTANGLE ( .color = 0xF8F8E1 ) )
    {
        RIPPLE( FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ) )
        {
            RIPPLE( FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                RIPPLE( FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ) ) );

                RIPPLE( FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = STATE().hovered ? 0xFFD5EE : 0xFFC1DA ) );

                RIPPLE( RECTANGLE ( .color = STATE().hovered ? 0xFFA4CF : 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( FORM( .width = FIXED(150), .height = FIXED(150) ), RECTANGLE ( .color = STATE().hovered ? 0x4D525A : 0x393E46 ) );
            );
        }

        RIPPLE( RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );

            RIPPLE( DISTURBANCE )
            {
                for(u32 i = 0; i < 50; i++)
                {
                    if (i) RIPPLE( RECTANGLE( .color = STATE().hovered ? 0xBAF9FF : 0x8ACCD5 ));
                    RIPPLE( FORM( .width = FIXED(4) ) );
                }
            }

            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );
        }
    }
}

void number_test()
{
    RIPPLE( RECTANGLE( .color = 0xffffff ) )
    {
        for (int i = 0; i < 200; i++)
        {
            RIPPLE( RECTANGLE( .color = 0x0 ) );
            RIPPLE( DISTURBANCE );
        }
    }
}

void rgb_test()
{
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0xff0000 : 0x000000 ) );
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0x00ff00 : 0x000000 ) );
    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0x0000ff : 0x000000 ) );
}

void element_func_helper( u32 color, u32 hovered_color )
{
    RIPPLE( RECTANGLE( .color = STATE().hovered ? hovered_color : color ) );
}

void element_func_test()
{
    element_func_helper(0xaa0000, 0xff0000);
    element_func_helper(0x00aa00, 0x00ff00);
    element_func_helper(0x0000aa, 0x0000ff);
}

typedef void (test_func)();
test_func* test_funcs[] = { &full_test, &number_test, &rgb_test, &element_func_test };

int main(int argc, char* argv[])
{
    u32 test_index = 0;
    while( !SURFACE_SHOULD_CLOSE() )
    {
        SURFACE( .title = "surface", .width = 800, .height = 800 + 45 )
        {
            RIPPLE( FORM( .direction = cld_VERTICAL ) )
            {
                RIPPLE( FORM( .height = FIXED(45) ) )
                {
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = STATE().hovered ? 0x0 : 0xff0000 ) ) { if (STATE().hovered) test_index = 0; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = STATE().hovered ? 0x0 : 0x00ff00 ) ) { if (STATE().hovered) test_index = 1; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = STATE().hovered ? 0x0 : 0x0000ff ) ) { if (STATE().hovered) test_index = 2; }
                    RIPPLE( FORM( .width = FIXED(45) ), RECTANGLE( .color = STATE().hovered ? 0x0 : 0x00ffaa ) ) { if (STATE().hovered) test_index = 3; }
                }

                RIPPLE( DISTURBANCE )
                {
                    test_index = min(test_index, sizeof(test_funcs));
                    test_funcs[test_index]();
                }
            }
        }
        //height += 2;
        //if (height > 800) height = 1;
    }

    return 0;
}
