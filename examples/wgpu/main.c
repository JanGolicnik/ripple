/*
#include "marrow/mapa.h"
#include "printccy/printccy.h"
#include <stdio.h>

i32 main()
{
    MAPA(char*, i32) ii_map;
    mapa_init(ii_map, mapa_hash_djb2, mapa_cmp_bytes, nullptr);

    char* keys[] = { "mister", "beast", "awoo", "heyy", "yaho !", "id:3" };

    // i32 keys[] = {
    //     0, 1, 1, 2, 3, 5, 8, 13, 21, 34,
    //     55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181
    // };

    i32 values[] = {
        0, 1, 1, 4, 9, 25, 64, 169, 441, 1156,
        3025, 7921, 20736, 54289, 142129, 372100, 974169, 2550409, 6677056, 17480761
    };

    for (u32 i = 0; i < array_len(keys); i++)
    {
        (void)mapa_insert(ii_map, &keys[i], values[i]);
    }

    for (u32 i = 0; i < array_len(keys); i++)
    {
        i32 value = *mapa_get(ii_map, &keys[i]);
        if (value != values[i])
            abort("failed at {}", value);
    }

    for (u32 i = 0; i < ii_map.size; i++)
    {
        if (!ii_map.entries[i].has_value)
        {
            printf("[%d]: (X) -> (X)\n", i);
            continue;
        }
        char* key = ii_map.entries[i].key;
        i32 value = ii_map.entries[i].value;
        printf("[%d]: (%s) -> (%d)\n", i, key, value);
    }

    for (u32 i = 0; i < array_len(keys); i++)
    {
        u64 index = mapa_get_index(ii_map, &keys[i]);
        if (index >= ii_map.size) continue;
        mapa_remove_at_index(ii_map, index);
        // // TODO: this somehow modifies the size??
        // printout("size after removing is: {}\n", ii_map.size); 
    }

    for (u32 i = 0; i < ii_map.size; i++)
    {
        if (!ii_map.entries[i].has_value)
        {
            printf("[%d]: (X) -> (X)\n", i);
            continue;
        }
        char* key = ii_map.entries[i].key;
        i32 value = ii_map.entries[i].value;
        printf("[%d]: (%s) -> (%d)\n", i, key, value);
    }

    return 0;
}
*/

/*
#include <marrow/vektor.h>
#include <stdio.h>

int main() {
    VEKTOR(int) vec;
    vektor_init(vec, 0, nullptr);

    vektor_add(vec, 1);
    vektor_add(vec, 2);

    for (u64 i = 0; i < vec.n_items; ++i)
        printout("vec[{}] = {}\n", i, vec.items[i]);

    return 0;

    vektor_insert(vec, 5, 999);  // Insert 999 at position 5
    vektor_remove(vec, 2);       // Remove element at index 2

    for (u64 i = 0; i < vec.n_items; ++i)
        printout("vec[{}] = {}\n", i, vec.items[i]);

    vektor_free(vec);
    return 0;
}
*/

#include <marrow/marrow.h>
#include <glfw/glfw3.h>

#define RIPPLE_RENDERING_WGPU
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_wgpu.h"

void full_test(f32 dt, RippleImage image)
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
                    RIPPLE( FORM( .width = FIXED((i32)((f32)w * t)), .height = FIXED((i32)((f32)h * t)), .min_width = FIXED(150), .min_height = FIXED(150) ),
                            IMAGE( .image = image ) ){
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

typedef struct {
    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        RippleImage image;
    } target;
} Context;

Context create_context(WGPUQueue queue, WGPUDevice device)
{
    Context ctx = { 0 };

    u32 texture_size = 800;
    ctx.target.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
            .label = "render_target",
            .size = (WGPUExtent3D){ .width = texture_size, .height = texture_size, .depthOrArrayLayers = 1 },
            .format = WGPUTextureFormat_RGBA8Unorm,
            .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment,
            .dimension = WGPUTextureDimension_2D,
            .mipLevelCount = 1,
            .sampleCount = 1
        });

    ctx.target.view = wgpuTextureCreateView(ctx.target.texture, &(WGPUTextureViewDescriptor){
            .format = WGPUTextureFormat_RGBA8Unorm,
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        });

    ctx.target.image = ripple_register_image(ctx.target.view);

    return ctx;
}

int main(int argc, char* argv[])
{
    RippleBackendConfig render_config = ripple_get_default_backend_config();
    ripple_backend_initialize(render_config);
    WGPUQueue queue = wgpuDeviceGetQueue(render_config.device);

    Context ctx = create_context(queue, render_config.device);

    bool main_is_open = true;

    f32 font_size = 64.0f;

    Allocator text_allocator;
    unused text_allocator;
    u8 text_arena[1024];

    i32 small_window_x = 0;
    i32 small_window_y = 0;

    while (main_is_open) {
        buf_set(text_arena, 0, sizeof(text_arena));
        LinearAllocatorContext linear_context = (LinearAllocatorContext){.data = text_arena, sizeof(text_arena)};
        text_allocator = (Allocator){.context = (void*)&linear_context, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free };

        SURFACE( .title = "surface", .width = 800, .height = 800, .is_open = &main_is_open )
        {
            RIPPLE( IMAGE( .image = ctx.target.image ))
            {
            }
        }

        SURFACE( .title = "hellooo", .width = 300, .height = 300, .x = &small_window_x, .y = &small_window_y, .hide_title = true )
        {
            RIPPLE( FORM( .direction = cld_VERTICAL ) )
            {
                RIPPLE( RECTANGLE( .color = 0x3e3e3e ), FORM( .height = FIXED(32) ) )
                {
                    static i32 mouse_offset_x = 0;
                    static i32 mouse_offset_y = 0;
                    if (STATE().clicked)
                    {
                        mouse_offset_x = CURSOR().x;
                        mouse_offset_y = CURSOR().y;
                    }

                    if (STATE().is_weak_held)
                    {
                        small_window_x += CURSOR().x - mouse_offset_x;
                        small_window_y += CURSOR().y - mouse_offset_y;
                    }
                }

                CENTERED(
                    const char* text = "helo";
                    i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                    RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text, .font_size = font_size ));
                );
            }
        }

        RIPPLE_RENDER_END(RIPPLE_RENDER_BEGIN());
    }

    // CLEAN UP WGPU

    //wgpuPipelineLayoutRelease(pipeline_layout);
    //wgpuBindGroupLayoutRelease(bind_group_layout);
    //wgpuBindGroupRelease(bind_group);

    //wgpuBufferRelease(vertex_buffer);
    //wgpuBufferRelease(index_buffer);
    //wgpuBufferRelease(uniform_buffer);

    //wgpuRenderPipelineRelease(pipeline);

    //wgpuQueueRelease(queue);

    //wgpuDeviceRelease(device);

    //wgpuSurfaceUnconfigure(surface);

    //wgpuInstanceRelease(instance);

    //// CLEAN UP GLFW

    //glfwDestroyWindow(window);

    return 0;
}
