#define debug(...) (void)0

#define RIPPLE_RENDERING_RAYLIB
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "ripple.h"

// heirarchy or growing elements, some fixed some not
void flex_test(void)
{
    RIPPLE( UNNAMED,
            FORM ( .direction = cld_VERTICAL ),
            RECTANGLE ( .color = 0xF8F8E1 )
    ){
        RIPPLE( LINE_UNIQUE_HASH, FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ) )
        {
            RIPPLE( LINE_UNIQUE_HASH, FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                RIPPLE( LINE_UNIQUE_HASH, FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ) ) );

                RIPPLE( LINE_UNIQUE_HASH, FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = STATE().hovered ? 0xFFD5EE : 0xFFC1DA ) );

                RIPPLE( LINE_UNIQUE_HASH, RECTANGLE ( .color = STATE().hovered ? 0xFFA4CF : 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( LINE_UNIQUE_HASH, FORM( .width = FIXED(150), .height = FIXED(150) ), RECTANGLE ( .color = STATE().hovered ? 0x4D525A : 0x393E46 ) );
            );
        }

        RIPPLE( UNNAMED, RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( UNNAMED, FORM( .height = DEPTH(.1f, FOUNDATION) ) );

            RIPPLE( UNNAMED, DISTURBANCE )
            {
                for(u32 i = 0; i < 50; i++)
                {
                    if (i) RIPPLE( LINE_UNIQUE_HASH + i * 2, RECTANGLE( .color = STATE().hovered ? 0xBAF9FF : 0x8ACCD5 ));
                    RIPPLE( UNNAMED, FORM( .width = FIXED(4) ) );
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
            RIPPLE( UNNAMED, RECTANGLE( .color = 0x0 ) );
            RIPPLE( UNNAMED, DISTURBANCE );
        }
    }
}

void rgb_test()
{
    RIPPLE( LABEL(1), RECTANGLE( .color = STATE().hovered ? 0xff0000 : 0x000000 ) );
    RIPPLE( LABEL(2), RECTANGLE( .color = STATE().hovered ? 0x00ff00 : 0x000000 ) );
    RIPPLE( LABEL(3), RECTANGLE( .color = STATE().hovered ? 0x0000ff : 0x000000 ) );
}

int main(int argc, char* argv[])
{
    u32 height = 650;
    while( !SURFACE_SHOULD_CLOSE() )
    {
        SURFACE( .title = "surface", .width = 800, .height = height )
        {
            flex_test();
        }

        //height += 2;
        //if (height > 800) height = 1;
    }

    return 0;
}
