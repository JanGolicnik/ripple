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

    mat4s proj;

    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    WGPUBuffer instance_buffer;

    WGPURenderPipeline pipeline;
    WGPUPipelineLayout pipeline_layout;

    WGPUBindGroup bind_group;
    WGPUBindGroupLayout bind_group_layout;
    WGPUBuffer uniform_buffer;
} Context;

typedef struct {
    mat4 camera_matrix;
} ShaderData;

typedef struct {
    vec4 position;
    vec4 normal;
} Vertex;

typedef struct {
    vec4 color;
} Instance;

static const Vertex vertices[] = {
    { .position = {-0.5f,-0.5f,-0.5f,1.0f}, .normal = {-INV_SQRT_3,-INV_SQRT_3,-INV_SQRT_3,0.0f} },
    { .position = { 0.5f,-0.5f,-0.5f,1.0f}, .normal = { INV_SQRT_3,-INV_SQRT_3,-INV_SQRT_3,0.0f} },
    { .position = { 0.5f, 0.5f,-0.5f,1.0f}, .normal = { INV_SQRT_3, INV_SQRT_3,-INV_SQRT_3,0.0f} },
    { .position = {-0.5f, 0.5f,-0.5f,1.0f}, .normal = {-INV_SQRT_3, INV_SQRT_3,-INV_SQRT_3,0.0f} },
    { .position = {-0.5f,-0.5f, 0.5f,1.0f}, .normal = {-INV_SQRT_3,-INV_SQRT_3, INV_SQRT_3,0.0f} },
    { .position = { 0.5f,-0.5f, 0.5f,1.0f}, .normal = { INV_SQRT_3,-INV_SQRT_3, INV_SQRT_3,0.0f} },
    { .position = { 0.5f, 0.5f, 0.5f,1.0f}, .normal = { INV_SQRT_3, INV_SQRT_3, INV_SQRT_3,0.0f} },
    { .position = {-0.5f, 0.5f, 0.5f,1.0f}, .normal = {-INV_SQRT_3, INV_SQRT_3, INV_SQRT_3,0.0f} },
};

static const uint16_t indices[] = {
    4,5,6,  4,6,7,
    1,0,3,  1,3,2,
    0,4,7,  0,7,3,
    5,1,2,  5,2,6,
    3,7,6,  3,6,2,
    0,1,5,  0,5,4
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

        ctx.proj = glms_ortho(-5.0f, +5.0f, -5.0f, +5.0f, 0.1f, 1000.0f);

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
                                    .format = WGPUVertexFormat_Float32x4,
                                    .offset = 0
                                },
                                [1] = {
                                    .shaderLocation = 1,
                                    .format = WGPUVertexFormat_Float32x4,
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

        ctx.vertex_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
            .size = sizeof(vertices),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            });
        wgpuQueueWriteBuffer(queue, ctx.vertex_buffer, 0, vertices, sizeof(vertices));

        ctx.instance_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
                .size = sizeof(instances),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            });
        wgpuQueueWriteBuffer(queue, ctx.instance_buffer, 0, instances, sizeof(instances));

        ctx.index_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
            .size = sizeof(indices),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
            });
        wgpuQueueWriteBuffer(queue, ctx.index_buffer, 0, indices, sizeof(indices));
    }

    return ctx;
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
    u32 n_loops = 0;

    BumpAllocator str_allocator = bump_allocator_create();

    while (main_is_open) {
        f32 time = glfwGetTime();
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

        {
            mat4s view = glms_lookat(
                (vec3s){ .x = sin(time) * 40.0f, .y = 40.0f, .z = cos(time) * 40.0f},
                (vec3s){ .x = 0.0f, .y = 0.0f, .z = 0.0f},
                GLMS_YUP
            );

            ShaderData shader_data = {0};
            glm_mat4_copy(glms_mat4_mul(ctx.proj, view).raw, shader_data.camera_matrix); // cam.raw is plain mat4
                                                               //
            wgpuQueueWriteBuffer(queue, ctx.uniform_buffer, 0, &shader_data, sizeof shader_data);
        }

        SURFACE( .title = S8("surface"), .width = 800, .height = 800, .is_open = &main_is_open )
        {
            s8 text = format("fps rn is: {.2f}", &str_allocator, 1.0f / (dt_accum / dt_samples));
            i32 w, h; ripple_measure_text(text, 32.0f, &w, &h);
            RIPPLE( FORM( .width = FIXED(w), .height = FIXED(h) ), WORDS( .text = text ));

            RIPPLE( IMAGE( ctx.target.view ) );
        }

        RippleRenderData render_data = RIPPLE_RENDER_BEGIN();

        {
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
                    .colorAttachmentCount = 1,
                    .colorAttachments = &(WGPURenderPassColorAttachment){
                        .view = ctx.target.view,
                        .loadOp = WGPULoadOp_Clear,
                        .storeOp = WGPUStoreOp_Store,
                        .clearValue = (WGPUColor){ 0.0f, 0.0f, 0.0f, 1.0f },
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
            wgpuRenderPassEncoderDrawIndexed(render_pass, array_len(indices), array_len(instances), 0, 0, 0);

            wgpuRenderPassEncoderEnd(render_pass);
        }

        RIPPLE_RENDER_END(render_data);

        bump_allocator_reset(&str_allocator);
    }

    return 0;
}
