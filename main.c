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
                        //.height = FIXED(100),
                        .height = DEPTH(1.0f, FOUNDATION),
                        .width = DEPTH(1.0f, FOUNDATION),
                        .min_width = DEPTH(1.0f, REFINEMENT),
                    ),
                    .accept_input = true,
                ),
                CONSEQUENCE (
                    .color = TREMBLING() ? 0xffffff : 0x123456
                )
        ){
            //RIPPLE("pattern",
            //        IDEA(0),
            //        PATTERN (
            //            .content = IS_TREMBLING("consequence") ? ";hey;" : ";;;;;",
            //            .font_size = FIXED(12)
            //        )
            //);
            RIPPLE("xd",
                    IDEA (
                        FORM (
                            .height = FIXED(100),
                        ),
                    ),
                    CONSEQUENCE (
                        .color = 0xaaaaaa
                    )
            ){}

            RIPPLE("xd2",
                    IDEA (
                        FORM (
                            .height = FIXED(100),
                        ),
                    ),
                    CONSEQUENCE (
                        .color = 0x2f2f2f
                    )
            ){
                RIPPLE("xd4",
                        IDEA ( 0 ),
                        CONSEQUENCE (
                            .color = 0xabcdef
                        )
                ){}

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
