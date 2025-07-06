#include <marrow/marrow.h>
#include <glfw/glfw3.h>

#define RIPPLE_RENDERING_WGPU
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_wgpu.h"

void full_test(f32 dt)
{
    static f32 time = 0.0f;
    time += dt;

    RIPPLE( FORM ( .direction = cld_VERTICAL ), RECTANGLE ( .color = 0xF8F8E1 ) )
    {
        RIPPLE( FORM ( .height = DEPTH(3.0f/4.0f, FOUNDATION) ) )
        {
            RIPPLE( FORM (.width = DEPTH(1.0f/3.0f, FOUNDATION), .direction = cld_VERTICAL ) )
            {
                f32 t = (sinf(time) + 1.0f) * 0.5f;
                RIPPLE( FORM (.height = DEPTH(t * (3.0f/4.0f), FOUNDATION ) ) );

                RIPPLE( FORM ( .width = DEPTH(.25f, FOUNDATION)), RECTANGLE ( .color = STATE().hovered ? 0xFFD5EE : 0xFFC1DA ) );

                RIPPLE( RECTANGLE ( .color = STATE().hovered ? 0xFFA4CF : 0xFF90BB ) );
            }

            RIPPLE()
            {
                u32 w = SHAPE().w;
                u32 h = SHAPE().h;

                CENTERED(
                    f32 t_target = h ? 150.0f / (f32)h : 0.0f;
                    static f32 t = -1.0f; if (t < 0.0f) t = t_target;
                    RIPPLE( FORM( .width = FIXED((i32)((f32)w * t)), .height = FIXED((i32)((f32)h * t)), .min_width = FIXED(150), .min_height = FIXED(150) ), RECTANGLE ( .color = STATE().hovered ? 0x4D525A : 0x393E46 ) ){
                        if (STATE().is_held)
                            t_target = 1.0f;
                    }
                    t += (t_target - t) * (1.0f - expf(-dt * 20.0f));
                );
            }
        }

        RIPPLE( RECTANGLE ( .color = 0x8ACCD5 ), FORM( .direction = cld_VERTICAL ) )
        {
            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );

            RIPPLE()
            {
                RippleElementLayoutConfig small_guys_form = { .width = FIXED(4) };
                for(u32 i = 0; i < 50; i++)
                {
                    RIPPLE( .layout = small_guys_form );
                    RIPPLE( RECTANGLE( .color = STATE().hovered ? 0xBAF9FF : 0x8ACCD5 ));
                }
                RIPPLE( .layout = small_guys_form );
            }

            RIPPLE( FORM( .height = DEPTH(.1f, FOUNDATION) ) );
        }
    }
}

int main(int argc, char* argv[])
{
    f32 accum_dt = 0.0f;
    u32 n_dt_samples = 0;
    f32 prev_time = 0.0f;
    while( SURFACE_IS_STABLE() )
    {
        f32 time = glfwGetTime();
        f32 dt = time - prev_time;
        prev_time = time;

        n_dt_samples += 1;
        accum_dt += dt;
        if (accum_dt > 1.0f)
        {
            debug("Running at {} fps", 1.0 / (f64)(accum_dt / (f32)n_dt_samples));
            accum_dt = 0.0f;
            n_dt_samples = 0;
        }

        SURFACE( .title = "surface", .width = 800, .height = 800 )
        {
            RIPPLE( RECTANGLE( .color = 0xffffff ) ){
                full_test(dt);
                continue;
                u32 w = SHAPE().w;
                u32 h = SHAPE().h;
                CENTERED (
                    RIPPLE( FORM( .width = FIXED( w / 2 ), .height = FIXED( h / 2 )), RECTANGLE ( .color = 0x0 ) );
                );
            }
        }

        void* context = RIPPLE_RENDER_BEGIN();
        RIPPLE_RENDER_END(context);
    }

    /*
    // CLEAN UP WGPU

    wgpuPipelineLayoutRelease(pipeline_layout);
    wgpuBindGroupLayoutRelease(bind_group_layout);
    wgpuBindGroupRelease(bind_group);

    wgpuBufferRelease(vertex_buffer);
    wgpuBufferRelease(index_buffer);
    wgpuBufferRelease(uniform_buffer);

    wgpuRenderPipelineRelease(pipeline);

    wgpuQueueRelease(queue);

    wgpuDeviceRelease(device);

    wgpuSurfaceUnconfigure(surface);

    wgpuInstanceRelease(instance);

    // CLEAN UP GLFW

    glfwDestroyWindow(window);
     */

    return 0;
}
