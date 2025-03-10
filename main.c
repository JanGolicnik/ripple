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
                        .height = FIXED(100),
                        .width = DEPTH(0.33f, FOUNDATION),
                        .min_width = DEPTH(1.0f, REFINEMENT),
                    ),
                    .accept_input = true,
                ),
                CONSEQUENCE (
                    .color = TREMBLING() ? 0xffffff : 0xaaaaaa
                )
        ){
            RIPPLE("pattern",
                    IDEA(0),
                    PATTERN (
                        .content = IS_TREMBLING("consequence") ? ";hey;" : ";;;;;",
                        .font_size = FIXED(12)
                    )
            );
        }

        if (IS_INTERACTION()) {
            RIPPLE("this is it", IDEA(0), ACCEPTANCE);
        }
    }

    return 0;
}
