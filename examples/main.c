#define debug(...) (void)0

#define RIPPLE_RENDERING_RAYLIB
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "ripple.h"

// heirarchy or growing elements, some fixed some not
void flex_test(void)
{
    RIPPLE( LABEL("1"),
            FORM (
                .direction = cld_VERTICAL,
                .height = DEPTH(1.0f, FOUNDATION),
                .width = DEPTH(1.0f, FOUNDATION),
                .min_width = DEPTH(1.0f, REFINEMENT),
            ),
            .accept_input = true,
            RECTANGLE ( .color = 0xF8F8E1 )
    ){
        RIPPLE( LABEL(2), FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ) )
        {
            //printout("hovered: {}\n", (bool)STATE().hovered);
            RIPPLE( LABEL(3), FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                RIPPLE( LABEL(123), FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ) ) );

                RIPPLE( LABEL(5), FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = 0xFFC1DA ) );

                RIPPLE( UNNAMED, RECTANGLE ( .color = 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( LABEL(6), FORM( .width = FIXED(150), .height = FIXED(150) ), RECTANGLE ( .color = 0x393E46 ) );
            );
        }

        RIPPLE( UNNAMED, RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( UNNAMED, DISTURBANCE );

            RIPPLE( UNNAMED, DISTURBANCE )
            {
                u32 n_elements = 12;
                for(u32 i = 0; i < n_elements; i++)
                {
                    if (i) RIPPLE( UNNAMED, RECTANGLE( .color = 0xa6e5ed ));
                    RIPPLE( UNNAMED, FORM( .width = FIXED(12) ) );
                }
            }

            RIPPLE( UNNAMED, FORM( .height = DEPTH(.1f, FOUNDATION) ) );
        }
    }
}

void number_test()
{
    RIPPLE( LABEL(0), RECTANGLE( .color = 0xffffff ) )
    {
        for (int i = 0; i < 200; i++)
        {
            RIPPLE( LABEL(i), RECTANGLE( .color = 0x0 ) );
            RIPPLE( UNNAMED, DISTURBANCE );
        }
    }
}

int main(int argc, char* argv[])
{
    u32 height = 650;
    while( !SURFACE_SHOULD_CLOSE() )
    {
        SURFACE( .title = "surface", .width = 800, .height = height )
        {
            RIPPLE( LABEL(1), RECTANGLE( .color = STATE().hovered ? 0xff0000 : 0x000000 ) );
            RIPPLE( LABEL(2), RECTANGLE( .color = STATE().hovered ? 0x00ff00 : 0x000000 ) );
            RIPPLE( LABEL(3), RECTANGLE( .color = STATE().hovered ? 0x0000ff : 0x000000 ) );
            //flex_test();
        }

        //height += 2;
        //if (height > 800) height = 1;
    }

    return 0;
}
