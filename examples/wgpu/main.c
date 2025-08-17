#include <marrow/marrow.h>
#include <glfw/glfw3.h>

#define RIPPLE_RENDERING_WGPU
#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_wgpu.h"

typedef struct {
    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        RippleImage image;
    } target;

    WGPUShaderModule shader;
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    u32 vertex_count;
} Context;

static WGPUBuffer create_buffer(WGPUDevice device, const void* data, size_t size, WGPUBufferUsageFlags usage, const char* label)
{
    return wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
        .label = label,
        .usage = usage | WGPUBufferUsage_CopyDst,
        .size  = size,
        .mappedAtCreation = false
    });
}

Context create_context(WGPUQueue queue, WGPUDevice device)
{
    Context ctx = (Context){0};

    u32 texture_size = 800;
    ctx.target.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
        .label = "render_target",
        .size  = (WGPUExtent3D){ .width = texture_size, .height = texture_size, .depthOrArrayLayers = 1 },
        .format = WGPUTextureFormat_RGBA8Unorm,
        .usage  = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment,
        .dimension = WGPUTextureDimension_2D,
        .mipLevelCount = 1,
        .sampleCount   = 1
    });
    ctx.target.view = wgpuTextureCreateView(ctx.target.texture, &(WGPUTextureViewDescriptor){
        .label = "render_target_view",
        .format = WGPUTextureFormat_RGBA8Unorm,
        .dimension = WGPUTextureViewDimension_2D,
        .mipLevelCount = 1,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    });
    ctx.target.image = ripple_register_image(ctx.target.view);

    ctx.shader = wgpuDeviceCreateShaderModule(device, &(WGPUShaderModuleDescriptor) {
        .label = "triangle_shader",
        .hintCount = 0,
        .hints = nullptr,
        .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
            .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
            .code =
                "struct VSOut { @builtin(position) pos: vec4<f32>, @location(0) color: vec3<f32> };\n"
                "@vertex fn vs_main(@location(0) in_pos: vec2<f32>, @location(1) in_col: vec3<f32>) -> VSOut {\n"
                "  var o: VSOut; o.pos = vec4<f32>(in_pos, 0.0, 1.0); o.color = in_col; return o; }\n"
                "@fragment fn fs_main(in: VSOut) -> @location(0) vec4<f32> { return vec4<f32>(in.color, 1.0); }\n"
        }
    });

    struct { f32 x, y, r, g, b; } vertices[] = {
        {  0.0f,  0.7f, 1.0f, 0.2f, 0.2f },
        { -0.7f, -0.7f, 0.2f, 1.0f, 0.2f },
        {  0.7f, -0.7f, 0.2f, 0.2f, 1.0f },
    };
    ctx.vertex_count = 3;

    ctx.vertex_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor){
        .label = "triangle_vb",
        .size  = sizeof(vertices),
        .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
    });
    wgpuQueueWriteBuffer(queue, ctx.vertex_buffer, 0, vertices, sizeof(vertices));

    ctx.pipeline = wgpuDeviceCreateRenderPipeline(device, &(WGPURenderPipelineDescriptor) {
        .label = "triangle_pipeline",
        .vertex = (WGPUVertexState){
            .module = ctx.shader,
            .entryPoint = "vs_main",
            .bufferCount = 1,
            .buffers = &(WGPUVertexBufferLayout) {
                .arrayStride = sizeof(vertices[0]),
                .stepMode    = WGPUVertexStepMode_Vertex,
                .attributeCount = 2,
                .attributes = (WGPUVertexAttribute[]) {
                    { .format = WGPUVertexFormat_Float32x2, .offset = 0, .shaderLocation = 0 },
                    { .format = WGPUVertexFormat_Float32x3, .offset = sizeof(f32) * 2, .shaderLocation = 1 },
                }
            }
        },
        .fragment = &(WGPUFragmentState) {
            .module = ctx.shader,
            .entryPoint = "fs_main",
            .targetCount = 1,
            .targets = &(WGPUColorTargetState) {
                .format = WGPUTextureFormat_RGBA8Unorm,
                .writeMask = WGPUColorWriteMask_All,
                .blend = &(WGPUBlendState) {
                    .color = { .operation=WGPUBlendOperation_Add, .srcFactor=WGPUBlendFactor_One, .dstFactor=WGPUBlendFactor_Zero },
                    .alpha = { .operation=WGPUBlendOperation_Add, .srcFactor=WGPUBlendFactor_One, .dstFactor=WGPUBlendFactor_Zero },
                }
            }
        },
        .primitive = (WGPUPrimitiveState){
            .topology = WGPUPrimitiveTopology_TriangleList,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None
        },
        .multisample = (WGPUMultisampleState){ .count = 1, .mask = ~0u, .alphaToCoverageEnabled = false },
    });

    return ctx;
}

bool button(const char* text, f32 font_size, u32 padding, RippleColor color, RippleColor highlight_color)
{
    bool clicked = false;
    i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
    RIPPLE( FORM( .min_width = DEPTH(1.0f, FOUNDATION), .width = FIXED((u32)text_w + padding), .height = FIXED((u32)text_h + padding)), RECTANGLE( .color = STATE().hovered ? highlight_color : color ) )
    {
        CENTERED(
            RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text ));
        );
        clicked = STATE().released;
    }

    return clicked;
}

int main(int argc, char* argv[])
{
    RippleBackendConfig render_config = ripple_get_default_backend_config();
    ripple_backend_initialize(render_config);
    WGPUQueue queue = wgpuDeviceGetQueue(render_config.device);

    Context ctx = create_context(queue, render_config.device);

    bool main_is_open = true;
    bool settings_is_open = false;
    bool fps_is_open = false;

    f32 font_size = 64.0f;

    Allocator text_allocator;
    u8 text_arena[1024];

    i32 small_window_x = 0;
    i32 small_window_y = 0;

    i32 fps_window_width = 300;
    i32 fps_window_height = 300;

    bool toolbar_is_hovered = false;
    f32 toolbar_pos = -100.0f;

    f32 prev_time = 0.0f;
    while (main_is_open) {
        f32 time = glfwGetTime();
        f32 dt = time - prev_time;
        prev_time = time;

        buf_set(text_arena, 0, sizeof(text_arena));
        LinearAllocatorContext linear_context = (LinearAllocatorContext){.data = text_arena, sizeof(text_arena)};
        text_allocator = (Allocator){.context = (void*)&linear_context, .alloc = &linear_allocator_alloc, .free = &linear_allocator_free };

        SURFACE( .title = "surface", .width = 800, .height = 800, .is_open = &main_is_open )
        {
            RIPPLE( IMAGE( .image = ctx.target.image ));

            f32 toolbar_target_pos = -100.0f;
            RIPPLE ( FORM ( .direction = cld_VERTICAL, .fixed = true, .width = DEPTH(1.0f, REFINEMENT), .height = DEPTH(1.0f, REFINEMENT), .x = FIXED( toolbar_pos )), RECTANGLE( .color = { 0x3e3e3e } ) )
            {
                toolbar_target_pos = -SHAPE().w;
                toolbar_is_hovered = STATE().hovered;
                for (u32 i = 0 ; i < 100; i++)
                {
                    if (button(settings_is_open ? ">settings<" : "settings", 32.0f, 5, RIPPLE_RGBA(0), RIPPLE_RGB(0x00ff00)))
                        settings_is_open = true;
                    if (button(fps_is_open ? ">fps<" : "fps", 32.0f, 5, RIPPLE_RGBA(0), RIPPLE_RGB(0x00ff00)))
                        fps_is_open = true;
                }

                if (toolbar_is_hovered || (CURSOR().y < SHAPE().h * 0.5f && CURSOR().x < 10))
                {
                    toolbar_target_pos = 0.0f;
                }
                toolbar_pos += (toolbar_target_pos - toolbar_pos) * (1.0f - expf(10.0f * -dt));
            }
        }

        if (settings_is_open)
        {
            SURFACE( .title = "settings", .width = 300, .height = 300, .x = &small_window_x, .y = &small_window_y, .is_open = &settings_is_open )
            {
                CENTERED(
                    const char* text = format("{}x{}", &text_allocator, small_window_x, small_window_y);
                    i32 text_w, text_h; ripple_measure_text((const char*)text, font_size, &text_w, &text_h);
                    RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text ));
                );
            }
        }

        if (fps_is_open)
        {
            SURFACE( .title = "fps", .width = fps_window_width, .height = fps_window_height, .is_open = &fps_is_open, .not_resizable = true )
            {
                CENTERED(
                    const char* text = format("fps: {.2f}", &text_allocator, 1.0f / dt);
                    i32 text_w, text_h; ripple_measure_text((const char*)text, 24.0f, &text_w, &text_h);
                    fps_window_width = text_w + 20;
                    fps_window_height = text_h + 20;
                    RIPPLE( FORM( .width = FIXED((u32)text_w), .height = FIXED((u32)text_h)), TEXT( .text = text ));
                );
            }
        }

        RippleRenderData render_data = RIPPLE_RENDER_BEGIN();

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
            .label = "triangle_pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment) {
                .view = ctx.target.view,
                .resolveTarget = NULL,
                .loadOp  = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = (WGPUColor){ .r = 0.06, .g = 0.06, .b = 0.08, .a = 1.0 }
            },
            .depthStencilAttachment = nullptr
        });

        wgpuRenderPassEncoderSetPipeline(pass, ctx.pipeline);
        wgpuRenderPassEncoderSetVertexBuffer(pass, 0, ctx.vertex_buffer, 0, WGPU_WHOLE_SIZE);
        wgpuRenderPassEncoderDraw(pass, ctx.vertex_count, 1, 0, 0);
        wgpuRenderPassEncoderEnd(pass);

        RIPPLE_RENDER_END(render_data);
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
