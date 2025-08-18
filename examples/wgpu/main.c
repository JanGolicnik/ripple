#include <windows.h>
#include <marrow/marrow.h>

#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_empty.h"

#include <time.h>

double now_seconds(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

typedef struct {
    int x, y, z;
} Test;

int main(int argc, char* argv[])
{
    ripple_backend_initialize(ripple_get_default_backend_config());

    bool main_is_open = true;

    f32 prev_time = 0.0f;
    f32 dt_accum = 0.0f;
    f32 dt_samples = 0.0f;
    u32 n_loops = 0;
    while (main_is_open) {
        f32 time = now_seconds();
        f32 dt = time - prev_time;
        if ((dt_accum += dt) > 1.0f)
        {
            debug("fps: {}", 1.0f / (dt_accum / dt_samples));
            dt_accum = 0.0f;
            dt_samples = 0.0f;
            if (n_loops++ > 20)
                break;
        }
        dt_samples += 1.0f;
        prev_time = time;

        SURFACE( .title = "surface", .width = 800, .height = 800, .is_open = &main_is_open )
        {
            for (i32 i = 0; i < 100000; i++)
            {
                RIPPLE( RECTANGLE( .color = { 0xff0000 } ));
            }
        }

        RIPPLE_RENDER_END(RIPPLE_RENDER_BEGIN());
    }

    return 0;
}
