#include "src/ripple.h"

int main(int argc, char* argv[])
{
    RIPPLE("Gumb1", BUTTON(
            .layout = {
                .centered = true,
                .width = GROW(PERCENT(.5f), PARENT),
                .height = FIXED(PIXELS(100)),
                .min_width = GROW(PERCENT(1.0f), CHILD)
            },
            .color = HOVERED() ? 0xffffff : 0xaaaaaa
    ))
    {
    }
    return 0;
}
