#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "src/ripple.h"

int main(int argc, char* argv[])
{
    SURFACE("surface", .width = 800, .height = 300,
            .cursor_data = (RippleCursorData) {.x = 100, .y = 100, .left_click = true},
    )
    {
        RIPPLE("consequence",
                IDEA (
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
            RIPPLE("xd",
                    IDEA (
                        FORM (
                            .height = FIXED(200),
                        ),
                    ),
                    CONSEQUENCE (
                        .color = 0xaaaaaa
                    )
            ){}

            RIPPLE("xd2",
                    IDEA ( 0 ),
                    CONSEQUENCE (
                        .color = 0x2f2f2f
                    )
            ){
                RIPPLE("xd4",
                       IDEA ( FORM ( .child_layout_direction = cld_VERTICAL ) ),
                        CONSEQUENCE (
                            .color = 0xabcdef
                        )
                ){
                    RIPPLE("xd6",
                        IDEA ( 0 ),
                            CONSEQUENCE (
                                .color = 0x262c36
                            )
                    ){}

                    RIPPLE("xd7",
                            IDEA ( 0 ),
                            CONSEQUENCE (
                                .color = 0x5c9475
                            )
                    ){}
                }

                RIPPLE("xd5",
                        IDEA ( 0 ),
                        CONSEQUENCE (
                            .color = 0x4f3e2d
                        )
                ){}

            }

            RIPPLE("xd3",
                    IDEA ( 0 ),
                    CONSEQUENCE (
                        .color = 0xabcdef
                    )
            ){}
        }
    }

    return 0;
}
