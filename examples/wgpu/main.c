#include <marrow/marrow.h>

#define RIPPLE_IMPLEMENTATION
#define RIPPLE_WIDGETS
#include "backends/ripple_wgpu.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct.h>

typedef struct {
    struct {
        WGPUTexture texture;
        WGPUTextureView view;
    } target;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
    } depth;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        WGPUExtent3D extent;
    } gradient;

    mat4s proj;

    u32 n_vertices;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    u32 n_indices;
    WGPUBuffer instance_buffer;

    WGPURenderPipeline pipeline;
    WGPUPipelineLayout pipeline_layout;

    WGPUBindGroup bind_group;
    WGPUBindGroupLayout bind_group_layout;
    WGPUBuffer uniform_buffer;
} Context;

typedef struct {
    mat4 camera_matrix;
    vec3s camera_position;
    f32 time;
    f32 gain;
    f32 lacunarity;
    f32 scale;
} ShaderData;

typedef struct {
    vec3s position;
    vec3s normal;
} Vertex;

typedef struct {
    vec4 color;
} Instance;


Vertex icosahedron_vertices[] = {
    { .position = {{ 0.000000, -1.000000,  0.000000 }} },
    { .position = {{ 0.723600, -0.447215,  0.525720 }} },
    { .position = {{ -0.276385, -0.447215,  0.850640 }} },
    { .position = {{ -0.894425, -0.447215,  0.000000 }} },
    { .position = {{ -0.276385, -0.447215, -0.850640 }} },
    { .position = {{ 0.723600, -0.447215, -0.525720 }} },
    { .position = {{ 0.276385,  0.447215,  0.850640 }} },
    { .position = {{ -0.723600,  0.447215,  0.525720 }} },
    { .position = {{ -0.723600,  0.447215, -0.525720 }} },
    { .position = {{ 0.276385,  0.447215, -0.850640 }} },
    { .position = {{ 0.894425,  0.447215,  0.000000 }} },
    { .position = {{ 0.000000,  1.000000,  0.000000 }} },
};

u16 icosahedron_indices[] = {
    0, 1, 2,
    1, 0, 5,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5,
    1, 5, 10,
    2, 1, 6,
    3, 2, 7,
    4, 3, 8,
    5, 4, 9,
    1, 10, 6,
    2, 6, 7,
    3, 7, 8,
    4, 8, 9,
    5, 9, 10,
    6, 10, 11,
    7, 6, 11,
    8, 7, 11,
    9, 8, 11,
    10, 9, 11,
};

static const Instance instances[1] = {
    { .color = {1.0f, 0.0f, 0.0f, 1.0f} }
};

Context create_context(WGPUDevice device, WGPUQueue queue)
{
    Context ctx;

    {
       ctx.target.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
                .label = "surface_texture",
                .size = (WGPUExtent3D){ .width = 1000, .height = 1000, .depthOrArrayLayers = 1 },
                .format = WGPUTextureFormat_BGRA8UnormSrgb,
                .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        ctx.target.view = wgpuTextureCreateView(ctx.target.texture, &(WGPUTextureViewDescriptor){
                .format = WGPUTextureFormat_BGRA8UnormSrgb,
                .dimension = WGPUTextureViewDimension_2D,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
                .aspect = WGPUTextureAspect_All,
            });

       ctx.depth.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
                .label = "depth_texture",
                .size = (WGPUExtent3D){ .width = 1000, .height = 1000, .depthOrArrayLayers = 1 },
                .format = WGPUTextureFormat_Depth24Plus,
                .usage = WGPUTextureUsage_RenderAttachment,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        ctx.depth.view = wgpuTextureCreateView(ctx.depth.texture, &(WGPUTextureViewDescriptor){
                .format = WGPUTextureFormat_Depth24Plus,
                .dimension = WGPUTextureViewDimension_2D,
                .aspect = WGPUTextureAspect_DepthOnly,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
            });

       ctx.gradient.extent = (WGPUExtent3D){ .width = 256, .height = 1, .depthOrArrayLayers = 1 };

       ctx.gradient.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
                .label = "gradient_texture",
                .size = ctx.gradient.extent,
                .format = WGPUTextureFormat_R32Float,
                .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        ctx.gradient.view = wgpuTextureCreateView(ctx.gradient.texture, &(WGPUTextureViewDescriptor){
                .format = WGPUTextureFormat_R32Float,
                .dimension = WGPUTextureViewDimension_2D,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
                .aspect = WGPUTextureAspect_All,
            });
    }

    {
        ctx.bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &(WGPUBindGroupLayoutDescriptor){
            .entryCount = 1,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                    .buffer = {
                        .type = WGPUBufferBindingType_Uniform,
                        .minBindingSize = sizeof(ShaderData)
                    }
                }
            }
        });

        ctx.uniform_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor){
            .size = sizeof(ShaderData),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

        ctx.proj = glms_ortho(-2.5f, +2.5f, -2.5f, +2.5f, 0.1f, 1000.0f);

        ctx.bind_group = wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor){
            .layout = ctx.bind_group_layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .buffer = ctx.uniform_buffer,
                    .offset = 0,
                    .size = sizeof(ShaderData)
                }
            }
        });

        FILE *fp = fopen("shader.wgsl", "rb");
        fseek(fp, 0, SEEK_END);
        u64 len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char shader_code[len];
        fread(shader_code, 1, len, fp);
        shader_code[len] = 0;
        fclose(fp);

        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &(WGPUShaderModuleDescriptor){
                .label = " descriptor",
                .hintCount = 0,
                .hints = nullptr,
                .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
                    .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
                    .code = shader_code
                }
            });

        ctx.pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &(WGPUPipelineLayoutDescriptor){
                .bindGroupLayoutCount = 1,
                .bindGroupLayouts = (WGPUBindGroupLayout[]) {
                    [0] = ctx.bind_group_layout
                }
            });

        WGPUStencilFaceState keepAlways = {
            .compare = WGPUCompareFunction_Always,
            .failOp = WGPUStencilOperation_Keep,
            .depthFailOp = WGPUStencilOperation_Keep,
            .passOp = WGPUStencilOperation_Keep
        };

        ctx.pipeline = wgpuDeviceCreateRenderPipeline(_context.config.device, &(WGPURenderPipelineDescriptor){
                .vertex = {
                    .module = shader_module,
                    .entryPoint = "vs_main",
                    .bufferCount = 2,
                    .buffers = (WGPUVertexBufferLayout[]) {
                        [0] = {
                            .attributeCount = 2,
                            .attributes = (WGPUVertexAttribute[]) {
                                [0] = {
                                    .shaderLocation = 0,
                                    .format = WGPUVertexFormat_Float32x3,
                                    .offset = 0
                                },
                                [1] = {
                                    .shaderLocation = 1,
                                    .format = WGPUVertexFormat_Float32x3,
                                    .offset = offsetof(Vertex, normal)
                                }
                            },
                            .arrayStride = sizeof(Vertex),
                            .stepMode = WGPUVertexStepMode_Vertex,
                        },
                        [1] = {
                            .attributeCount = 1,
                            .attributes = (WGPUVertexAttribute[]) {
                                [0] = {
                                    .shaderLocation = 2,
                                    .format = WGPUVertexFormat_Float32x4,
                                    .offset = 0
                                },
                            },
                            .arrayStride = sizeof(Instance),
                            .stepMode = WGPUVertexStepMode_Instance,
                        }
                    }
                },
                .primitive = {
                    .topology = WGPUPrimitiveTopology_TriangleList,
                    /* .topology = WGPUPrimitiveTopology_TriangleStrip, */
                    .stripIndexFormat = WGPUIndexFormat_Undefined,
                    .frontFace = WGPUFrontFace_CCW,
                    .cullMode = WGPUCullMode_None
                },
                .fragment = &(WGPUFragmentState) {
                    .module = shader_module,
                    .entryPoint = "fs_main",
                    .targetCount = 1,
                    .targets = &(WGPUColorTargetState) {
                        .format = WGPUTextureFormat_BGRA8UnormSrgb,
                        .writeMask = WGPUColorWriteMask_All,
                        .blend = &(WGPUBlendState) {
                            .color =  {
                                .srcFactor = WGPUBlendFactor_SrcAlpha,
                                .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
                                .operation = WGPUBlendOperation_Add,
                            },
                            .alpha =  {
                                .srcFactor = WGPUBlendFactor_Zero,
                                .dstFactor = WGPUBlendFactor_One,
                                .operation = WGPUBlendOperation_Add,
                            }
                        },
                    }
                },
                .multisample = {
                    .count = 1,
                    .mask = ~0u,
                },
                .layout = ctx.pipeline_layout,
                .depthStencil = &(WGPUDepthStencilState) {
                    .format = WGPUTextureFormat_Depth24Plus,
                    .depthWriteEnabled = true,
                    .depthCompare = WGPUCompareFunction_Less,
                    .stencilFront = keepAlways,
                    .stencilBack = keepAlways,
                    .stencilReadMask = ~0,
                    .stencilWriteMask = ~0,
                }
            });

        wgpuShaderModuleRelease(shader_module);

        ctx.n_vertices = (array_len(icosahedron_indices) / 3) * 6;
        Vertex vertices[ctx.n_vertices];
        buf_copy(vertices, icosahedron_vertices, sizeof(icosahedron_vertices));
        u32 vertices_i = array_len(icosahedron_vertices);
        ctx.n_indices = array_len(icosahedron_indices) * 4;
        u16 indices[ctx.n_indices];
        u32 indices_i = 0;

        for (u32 i = 0; i < array_len(icosahedron_indices); i+= 3)
        {
            u16 i1 = icosahedron_indices[i + 0];
            u16 i3 = icosahedron_indices[i + 1];
            u16 i5 = icosahedron_indices[i + 2];
            Vertex v1 = icosahedron_vertices[i1];
            Vertex v3 = icosahedron_vertices[i3];
            Vertex v5 = icosahedron_vertices[i5];

            Vertex v2 = { .position = glms_vec3_scale(glms_vec3_add(v1.position, v3.position), 0.5f) };
            Vertex v4 = { .position = glms_vec3_scale(glms_vec3_add(v3.position, v5.position), 0.5f) };
            Vertex v6 = { .position = glms_vec3_scale(glms_vec3_add(v5.position, v1.position), 0.5f) };

            u16 i2 = vertices_i; vertices[vertices_i++] = v2;
            u16 i4 = vertices_i; vertices[vertices_i++] = v4;
            u16 i6 = vertices_i; vertices[vertices_i++] = v6;

            indices[indices_i++] = i1; indices[indices_i++] = i2; indices[indices_i++] = i6;
            indices[indices_i++] = i2; indices[indices_i++] = i3; indices[indices_i++] = i4;
            indices[indices_i++] = i4; indices[indices_i++] = i5; indices[indices_i++] = i6;
            indices[indices_i++] = i6; indices[indices_i++] = i2; indices[indices_i++] = i4;
        }

        debug("ctx.n_indices: {}, indices_i {}", ctx.n_indices, indices_i);
        debug("ctx.n_vertices {}, n_vertices: {}", ctx.n_vertices, vertices_i);

        for_each(vertex, vertices, array_len(vertices))
        {
            vertex->position = vertex->normal = glms_vec3_normalize(vertex->position);
        }

        ctx.vertex_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
            .size = sizeof(vertices),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            });
        wgpuQueueWriteBuffer(queue, ctx.vertex_buffer, 0, vertices, sizeof(vertices));

        ctx.index_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
            .size = sizeof(indices),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            });
        wgpuQueueWriteBuffer(queue, ctx.index_buffer, 0, indices, sizeof(indices));

        ctx.instance_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
                .size = sizeof(instances),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            });
        wgpuQueueWriteBuffer(queue, ctx.instance_buffer, 0, instances, sizeof(instances));
    }

    return ctx;
}

typedef struct {
    f32 t;
    f32 value;
} FloatRampStop;

bool float_ramp(WGPUTextureView view, FloatRampStop* stops, u32 n_stops)
{
    bool shouldRecreate = false;

    u32 stop_w = 10;

    RIPPLE( FORM( .width = PIXELS(300), .height = PIXELS(32), .direction = cld_HORIZONTAL), IMAGE( .image = view ))
    {
        u32 x = SHAPE().x;
        u32 w = SHAPE().w;
        u32 empty_w = w - n_stops * stop_w;

        CENTERED_VERTICAL(
            RIPPLE( FORM( .direction = cld_HORIZONTAL ) )
            {
                for (u32 i = 0; i < n_stops; i++)
                {
                    f32 prev_t = i == 0 ? 0.0f : stops[i - 1].t;
                    f32 next_t = i == n_stops - 1 ? 1.0f : stops[i + 1].t;

                    RIPPLE( FORM( .width = PIXELS( (stops[i].t - prev_t) * empty_w ) ) );
                    RIPPLE( FORM( .width = PIXELS(10), .height = PIXELS(10) ), RECTANGLE( .color = {0xffffff}) )
                    {
                        if (STATE().is_weak_held)
                        {
                            stops[i].t = clamp(((f32)CURSOR().x - (f32)x) / (f32)w, prev_t, next_t);
                            shouldRecreate = true;
                        }
                    }
                }
            }
        );

    }

    return shouldRecreate;
}

void slider(const char* label, f32* value, f32 max, f32 min, Allocator* str_allocator)
{
    f32 font_size = 32.0f;

    RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = PIXELS(font_size), .direction = cld_HORIZONTAL ) )
    {
        s8 text = format("{}: {2.2f}", str_allocator, label, *value);
        i32 width; ripple_measure_text(text, font_size, &width, nullptr);
        RIPPLE( FORM( .width = PIXELS(width) ), WORDS( .text = text ));

        RIPPLE( FORM( .width = PIXELS(5) ) );

        RIPPLE( FORM( .width = PIXELS(115), .direction = cld_HORIZONTAL ))
        {
            f32 range = max - min;
            f32 t = clamp(( *value - min ) / range, 0.0f, 1.0f);
            u32 w = SHAPE().w;
            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS((1.0f - t) * (w - 15)) , .height = PIXELS(font_size * 0.5f)), RECTANGLE( .color = { 0 } ) );
            );

            u32 x = SHAPE().x;
            RIPPLE( FORM( .width = PIXELS(15)), RECTANGLE( .color = { 0xff0000 } ) )
            {
                if (STATE().is_weak_held)
                {
                    *value = (1.0f - (clamp((f32)CURSOR().x - (f32)x, 0.0f, (f32)w) / (f32)w)) * range + min;
                }
            }

            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS(t * (w - 15)), .height = PIXELS(font_size * 0.5f)), RECTANGLE( .color = { 0 } ) );
            );
        }
    }
}

int main(int argc, char* argv[])
{
    RippleBackendConfig config = ripple_get_default_backend_config();
    ripple_backend_initialize(config);

    WGPUQueue queue = wgpuDeviceGetQueue(config.device);

    Context ctx = create_context(config.device, queue);
    unused ctx;

    bool main_is_open = true;

    f32 prev_time = 0.0f;
    f32 dt_accum = 0.0f;
    f32 dt_samples = 0.0f;

    BumpAllocator str_allocator = bump_allocator_create();

    ShaderData shader_data = { .gain = 0.25f, .lacunarity = 2.3f, .scale = 100.0f };

    FloatRampStop gradient_stops[3];
    gradient_stops[0] = (FloatRampStop){ 0.0f, 0.0f };
    gradient_stops[1] = (FloatRampStop){ 0.5f, 0.5f };
    gradient_stops[2] = (FloatRampStop){ 1.0f, 1.0f };

    while (main_is_open) {
        f32 time = shader_data.time = glfwGetTime();
        f32 dt = shader_data.time - prev_time;
        if ((dt_accum += dt) > 1.0f)
        {
            debug("fps: {}", 1.0f / (dt_accum / dt_samples));
            dt_accum = 0.0f;
            dt_samples = 0.0f;
        }
        dt_samples += 1.0f;
        prev_time = time;

        {
            shader_data.camera_position = (vec3s){ .x = sin(time) * 40.0f, .y = sin(time) * 40.0f + 40.0f, .z = cos(time) * 40.0f};

            mat4s view = glms_lookat( shader_data.camera_position, (vec3s){ .x = 0.0f, .y = 2.0f, .z = 0.0f}, GLMS_YUP );
            glm_mat4_copy(glms_mat4_mul(ctx.proj, view).raw, shader_data.camera_matrix); // cam.raw is plain mat4
        }

        SURFACE( .title = S8("surface"), .width = 800, .height = 800, .is_open = &main_is_open )
        {
            s8 text = format("fps rn is: {.2f}", &str_allocator, 1.0f / (dt_accum / dt_samples));
            i32 w, h; ripple_measure_text(text, 32.0f, &w, &h);
            RIPPLE( FORM( .width = PIXELS(w), .height = PIXELS(h) ), WORDS( .text = text ));

            slider("gain", &shader_data.gain, 0.0f, 3.0f, (Allocator*)&str_allocator);
            slider("lacunarity", &shader_data.lacunarity, 0.0f, 10.0f, (Allocator*)&str_allocator);
            slider("scale", &shader_data.scale, 0.0f, 1000.0f, (Allocator*)&str_allocator);

            if (float_ramp(ctx.gradient.view, gradient_stops, array_len(gradient_stops)))
            {
                f32 buffer[256];
                u32 buffer_i = 0;
                i32 n_stops = array_len(gradient_stops);
                for (i32 i = 0; i < n_stops + 1; i++)
                {
                    FloatRampStop v0 = i == 0 ? (FloatRampStop){ 0.0f, gradient_stops[0].value } : gradient_stops[max(i - 1, 0)];
                    FloatRampStop v1 = i == n_stops ? (FloatRampStop){ 1.0f, gradient_stops[i - 1].value } : gradient_stops[i];
                    i32 n_steps = array_len(buffer) * (v1.t - v0.t);
                    for (i32 j = 0; j < n_steps; j++)
                    {
                        f32 t = (f32)j / (f32)n_steps;
                        buffer[buffer_i++] = v0.value * (1.0f - t)  + v1.value * t;
                    }
                }

                wgpuQueueWriteTexture(queue, &(WGPUImageCopyTexture){
                        .texture = ctx.gradient.texture,
                        .aspect = WGPUTextureAspect_All
                    },
                    buffer,
                    sizeof(buffer),
                    &(WGPUTextureDataLayout){
                        .bytesPerRow = sizeof(buffer),
                        .rowsPerImage = 1
                    },
                    &ctx.gradient.extent
                );
            }

            RIPPLE( IMAGE( ctx.target.view ) );
        }

        wgpuQueueWriteBuffer(queue, ctx.uniform_buffer, 0, &shader_data, sizeof(shader_data));

        RippleRenderData render_data = RIPPLE_RENDER_BEGIN();

        {
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
                    .colorAttachmentCount = 1,
                    .colorAttachments = &(WGPURenderPassColorAttachment){
                        .view = ctx.target.view,
                        .loadOp = WGPULoadOp_Clear,
                        .storeOp = WGPUStoreOp_Store,
                        .clearValue = (WGPUColor){ 0.73f, 0.86f, 0.89f, 1.0f },
                    },
                    .depthStencilAttachment = &(WGPURenderPassDepthStencilAttachment){
                        .view = ctx.depth.view,
                        .depthLoadOp = WGPULoadOp_Clear,
                        .depthClearValue = 1.0f,
                        .depthStoreOp = WGPUStoreOp_Store,
                    },
                    .occlusionQuerySet = 0
                });

            wgpuRenderPassEncoderSetPipeline(render_pass, ctx.pipeline);
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, ctx.vertex_buffer, 0, wgpuBufferGetSize(ctx.vertex_buffer));
            wgpuRenderPassEncoderSetIndexBuffer(render_pass, ctx.index_buffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(ctx.index_buffer));
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, ctx.instance_buffer, 0, wgpuBufferGetSize(ctx.instance_buffer));
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, ctx.bind_group, 0, NULL);
            wgpuRenderPassEncoderDrawIndexed(render_pass, ctx.n_indices, array_len(instances), 0, 0, 0);

            wgpuRenderPassEncoderEnd(render_pass);
        }

        RIPPLE_RENDER_END(render_data);

        bump_allocator_reset(&str_allocator);
    }

    return 0;
}
