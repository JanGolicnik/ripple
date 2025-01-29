#define RIPPLE_IMPLEMENTATION
#include "src/ripple.h"

f32 text_width(const char* text, f32 font_size){
    return (f32)strlen(text) * font_size;
}

u32 main(int argc, char* argv[])
{
    LAKE("moje jezero :D", .width = 800, .height = 300)
    {
        RIPPLE("ribica",
                WAVE
                (
                    .centered = true,
                    .height = BUBBLES(100),
                    .width = DEPTH(0.33f, PARENT),
                    .min_width = DEPTH(1.0f, CHILD),
                ),
                FISH
                (
                    .color = HOVERED() ? 0xffffff : 0xaaaaaa
                )
        )
        {
            bool ribica_hovered = HOVERED();

            RIPPLE("tok",
                   WAVE
                   (
                       .centered = true,
                       .height = DEPTH(1.0f, PARENT),
                   ),
                    CURRENT
                    (
                        .content = ribica_hovered ? "hey :D" : "xd",
                        .font_size = BUBBLES(12),
                    )
            ){}
        }
    }
    return 0;
}
