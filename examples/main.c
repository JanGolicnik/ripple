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
            RIPPLE( LABEL(3), FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                RIPPLE( LABEL(123), FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ) ) );

                RIPPLE( LABEL(5), FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = 0xFFC1DA ) );

                RIPPLE( RECTANGLE ( .color = 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( LABEL(6), FORM( .width = FIXED(150), .height = FIXED(150) ), RECTANGLE ( .color = 0x393E46 ) );
            );
        }

        RIPPLE( RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( DISTURBANCE );

            RIPPLE( DISTURBANCE )
            {
                u32 n_elements = 12;
                for(u32 i = 0; i < n_elements; i++)
                {
                    if (i) RIPPLE( RECTANGLE( .color = 0xa6e5ed ));
                    RIPPLE( FORM( .width = FIXED(12) ) );
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
            RIPPLE( LABEL(i), RECTANGLE( .color = 0x0 ) );
            RIPPLE( DISTURBANCE );
        }
    }
}

int main(int argc, char* argv[])
{
    u32 alloc_size = 4096;
    LinearAllocator allocator = (LinearAllocator){ .data = malloc(alloc_size), .data_size = alloc_size };

    SURFACE( .title = "surface", .width = 800, .height = 300,
            .cursor_data = (RippleCursorData){.x = 100, .y = 100, .left_click = true },
            .allocator = (Allocator){ .context = &allocator, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free })
    {
        // flex_test();

        number_test();
    }

    free(allocator.data);

    return 0;
}
