#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "src/ripple.h"

void flex_test()
{
    RIPPLE( IDEA (
                LABEL("1"),
                FORM (
                    .child_layout_direction = cld_VERTICAL,
                    //.height = FIXED(100),
                    .height = DEPTH(1.0f, FOUNDATION),
                    .width = DEPTH(1.0f, FOUNDATION),
                    .min_width = DEPTH(1.0f, REFINEMENT),
                ),
                .accept_input = true,
            ),
            CONSEQUENCE (
                .color = 0x0
            )
    ){
        RIPPLE( IDEA (
                    LABEL(2),
                    FORM (
                        .height = FIXED(200),
                    ),
                ),
                CONSEQUENCE (
                    .color = 0xaaaaaa
                )
        ){}

        RIPPLE( IDEA ( LABEL(3) ),
                CONSEQUENCE (
                    .color = 0x2f2f2f
                )
        ){
            RIPPLE( IDEA (
                        FORM ( .child_layout_direction = cld_VERTICAL )
                    ),
                    CONSEQUENCE (
                        .color = 0xabcdef
                    )
            ){
                RIPPLE( DISTURBANCE,
                        CONSEQUENCE (
                            .color = 0x262c36
                        )
                ){}

                RIPPLE( DISTURBANCE,
                        CONSEQUENCE (
                            .color = 0x5c9475
                        )
                ){}
            }

            RIPPLE( DISTURBANCE,
                    CONSEQUENCE (
                        .color = 0x4f3e2d
                    )
            ){}

        }

        RIPPLE( DISTURBANCE,
                CONSEQUENCE (
                    .color = 0xabcdef
                )
        ){}
    }
}

int main(int argc, char* argv[])
{
    SURFACE( .title = "surface", .width = 800, .height = 300,
            .cursor_data = (RippleCursorData) {.x = 100, .y = 100, .left_click = true},
    )
    {
        flex_test();
    }

    return 0;
}
