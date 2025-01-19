#define RIPPLE_DEFINE_MARROW_MAPA
#define MARROW_MAPA_IMPLEMENTATION
#include "src/ripple.h"

int main(int argc, char* argv[])
{
    LAKE("moje jezero :D")
    {
        RIPPLE("ribica",
                WAVE
                (
                    .centered = true,
                    .width = GROW(DEPTH(.5f), PARENT),
                    .height = FIXED(BUBBLES(100)),
                    .min_width = GROW(DEPTH(1.0f), CHILD)
                ),
                FISH
                (
                    .color = HOVERED() ? 0xffffff : 0xaaaaaa
                )
        )
        {
            RIPPLE("tok",
                   WAVE
                   (
                       .centered = true,
                       .width = GROW(DEPTH(1.0f), PARENT),
                       .height = GROW(DEPTH(1.0f), PARENT),
                   ),
                    CURRENT
                    (
                        .content = "hey :D",
                        .font_size = FIXED(BUBBLES(12)),
                    )
            ){}
        }
    }
    return 0;
}
