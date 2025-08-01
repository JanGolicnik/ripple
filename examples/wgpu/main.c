#include <marrow/marrow.h>
#include <glfw/glfw3.h>

#define RIPPLE_RENDERING_WGPU
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_wgpu.h"

int main(int argc, char* argv[])
{
    bool main_is_open = true;
    bool second_is_open = false;
    bool third_is_open = false;

    i32 first_click_count = 0;
    i32 second_click_count = 0;

    f32 font_size = 64.0f;

    Allocator text_allocator;
    u8 text_arena[1024];

    while (main_is_open) {
        buf_set(text_arena, 0, sizeof(text_arena));
        LinearAllocatorContext linear_context = (LinearAllocatorContext){.data = text_arena, sizeof(text_arena)};
        text_allocator = (Allocator){.context = (void*)&linear_context, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free };

        SURFACE( .title = "surface", .width = 800, .height = 800, .is_open = &main_is_open )
        {
            RIPPLE( RECTANGLE( .color = 0x0000ff ))
            {
                u32 size = print(0, 0, "{}", first_click_count);
                u8* text = allocator_alloc(&text_allocator, size + 1);
                print((void*)text, size, "{}", first_click_count);
                i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                CENTERED(
                    RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = (char*)text, .font_size = font_size));
                );

                if (STATE().clicked)
                {
                    first_click_count = 0;
                    second_is_open = !second_is_open;
                }
            }

            RIPPLE( RECTANGLE( .color = 0x00ff00 ))
            {
                u32 size = print(0, 0, "{}", second_click_count);
                u8* text = allocator_alloc(&text_allocator, size + 1);
                print((void*)text, size, "{}", second_click_count);
                i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                CENTERED(
                    RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = (char*)text, .font_size = font_size));
                );

                if (STATE().clicked)
                {
                    second_click_count = 0;
                    third_is_open = !third_is_open;
                }
            }

            if (second_is_open)
            {
                SURFACE( .title = "HELLO!", .width = 300, .height = 300, .is_open = &second_is_open )
                {
                    RIPPLE( RECTANGLE( .color = 0x0000ff ))
                    {
                        CENTERED(
                            const char* text = "blue++";
                            i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                            RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text, .font_size = font_size));
                        );

                        if (STATE().clicked)
                            first_click_count++;
                    }
                }
            }

            if (third_is_open)
            {
                SURFACE( .title = "hh", .width = 300, .height = 300, .is_open = &third_is_open )
                {
                    RIPPLE( RECTANGLE( .color = 0x00ff00 ))
                    {
                        CENTERED(
                            const char* text = "green++";
                            i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                            RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text, .font_size = font_size));
                        );

                        if (STATE().clicked)
                            second_click_count++;
                    }
                }
            }
        }

        RIPPLE_RENDER_END(RIPPLE_RENDER_BEGIN());
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
