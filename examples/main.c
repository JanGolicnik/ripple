#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "ripple.h"

#define CENTERED(...) do {\
        RIPPLE(DISTURBANCE, AETHER);\
        RIPPLE(IDEA( FORM( .direction = cld_VERTICAL ) ), AETHER) {\
            RIPPLE(DISTURBANCE, AETHER);\
            { __VA_ARGS__ }\
            RIPPLE(DISTURBANCE, AETHER);\
        }\
        RIPPLE(DISTURBANCE, AETHER);\
    } while(false)

// heirarchy or growing elements, some fixed some not
void flex_test(void)
{
    RIPPLE( IDEA (
                LABEL("1"),
                FORM (
                    .direction = cld_VERTICAL,
                    //.height = FIXED(100),
                    .height = DEPTH(1.0f, FOUNDATION),
                    .width = DEPTH(1.0f, FOUNDATION),
                    .min_width = DEPTH(1.0f, REFINEMENT),
                ),
                .accept_input = true,
            ),
            CONSEQUENCE ( .color = 0xF8F8E1 )
    ){
        RIPPLE( IDEA (
                    LABEL(2),
                    FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ),
                ),
                AETHER
        ){
            RIPPLE( IDEA ( LABEL(3), FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) ), AETHER
            ){
                RIPPLE( IDEA ( LABEL(123), FORM (.height = DEPTH(3.0f/4.0f, FOUNDATION ))), AETHER);

                RIPPLE( IDEA ( LABEL(5), FORM ( .width = DEPTH(.25f, FOUNDATION)) ),
                        CONSEQUENCE ( .color = 0xFFC1DA ) );

                RIPPLE( DISTURBANCE, CONSEQUENCE ( .color = 0xFF90BB ) );
            }

            CENTERED(
                RIPPLE( IDEA( LABEL(6), FORM( .width = FIXED(150), .height = FIXED(150) ) ),
                        CONSEQUENCE ( .color = 0x393E46 )
                );
            );
        }

        RIPPLE( DISTURBANCE, CONSEQUENCE ( .color = 0x8ACCD5 ) );
    }
}

int main(int argc, char* argv[])
{
    LinearAllocator allocator = (LinearAllocator){ .data = malloc(1024), .data_size = 1024 };

    SURFACE( .title = "surface", .width = 800, .height = 300,
            .cursor_data = (RippleCursorData){.x = 100, .y = 100, .left_click = true },
            .allocator = (Allocator){ .context = &allocator, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free })
    {
        flex_test();
    }

    free(allocator.data);

    return 0;
}
