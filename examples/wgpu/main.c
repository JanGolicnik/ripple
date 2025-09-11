#include <marrow/marrow.h>

#define RIPPLE_IMPLEMENTATION
#define RIPPLE_BACKEND RIPPLE_WGPU | RIPPLE_GLFW
#include "ripple.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/struct.h>

RippleColor dark = { 0x222831 };
RippleColor dark2 = { 0x393E46 };
RippleColor light = { 0xEEEEEE };
RippleColor accent = { 0x00ADB5 };

f32 font_size = 32.0f;

typedef struct {
    WGPUTexture texture;
    WGPUTextureView view;
    WGPUExtent3D extent;
} Texture;

typedef struct {
    Texture texture;
    WGPUBindGroupLayout layout;
    WGPUBindGroup group;
} BindableTexture;

typedef struct {
    Texture target;
    Texture depth;

    BindableTexture gradient;
    BindableTexture color;

    u32 n_vertices;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    u32 n_indices;
    WGPUBuffer instance_buffer;

    WGPURenderPipeline pipeline;
    WGPUPipelineLayout pipeline_layout;

    struct {
        WGPUBindGroup group;
        WGPUBindGroupLayout layout;
        WGPUBuffer buffer;
    } shader_data;

    struct {
        WGPUSampler sampler;
        WGPUBindGroupLayout layout;
        WGPUBindGroup group;
    } sampler;
} Context;

typedef struct {
    mat4 camera_matrix;
    vec3s camera_position;
    f32 time;
    f32 gain;
    f32 speed;
    f32 offset;
    f32 height;
    f32 time_scale;
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

void subdivide(Vertex* vertices, u32* n_vertices, u16* indices, u32* n_indices)
{
    u32 n = *n_indices;
    for (u32 i = 0; i < n; i+= 3)
    {
        u16 i1 = indices[i + 0];
        u16 i3 = indices[i + 1];
        u16 i5 = indices[i + 2];
        Vertex v1 = vertices[i1];
        Vertex v3 = vertices[i3];
        Vertex v5 = vertices[i5];

        Vertex v2 = { .position = glms_vec3_scale(glms_vec3_add(v1.position, v3.position), 0.5f) };
        Vertex v4 = { .position = glms_vec3_scale(glms_vec3_add(v3.position, v5.position), 0.5f) };
        Vertex v6 = { .position = glms_vec3_scale(glms_vec3_add(v5.position, v1.position), 0.5f) };

        u16 i2 = *n_vertices; vertices[(*n_vertices)++] = v2;
        u16 i4 = *n_vertices; vertices[(*n_vertices)++] = v4;
        u16 i6 = *n_vertices; vertices[(*n_vertices)++] = v6;

        indices[i] = i1; indices[i + 1] = i2; indices[i + 2] = i6;
        indices[(*n_indices)++] = i2; indices[(*n_indices)++] = i3; indices[(*n_indices)++] = i4;
        indices[(*n_indices)++] = i4; indices[(*n_indices)++] = i5; indices[(*n_indices)++] = i6;
        indices[(*n_indices)++] = i6; indices[(*n_indices)++] = i2; indices[(*n_indices)++] = i4;
    }

    for_each_n(vertex, vertices, *n_vertices)
    {
        vertex->position = vertex->normal = glms_vec3_normalize(vertex->position);
    }
}

Texture create_texture(WGPUDevice device, const char* label, u32 w, u32 h, WGPUTextureFormat format, WGPUTextureUsage usage, WGPUTextureAspect aspect)
{
    Texture tex;

    tex.extent = (WGPUExtent3D){ .width = w, .height = h, .depthOrArrayLayers = 1 };

    tex.texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
            .label = label,
            .size = tex.extent,
            .format = format,
            .usage = usage,
            .dimension = WGPUTextureDimension_2D,
            .mipLevelCount = 1,
            .sampleCount = 1
        });

    tex.view = wgpuTextureCreateView(tex.texture, &(WGPUTextureViewDescriptor){
            .format = format,
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = aspect,
        });

    return tex;
}

BindableTexture create_bindable_texture(WGPUDevice device, Texture texture, WGPUShaderStage visibility)
{
    BindableTexture tex = { .texture = texture };

    tex.layout = wgpuDeviceCreateBindGroupLayout(device, &(WGPUBindGroupLayoutDescriptor)
        {
            .entryCount = 1,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = visibility,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                }
            }
        });

    tex.group = wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor){
            .layout = tex.layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .textureView = tex.texture.view
                }
            }
        });

    return tex;
}

Context create_context(WGPUDevice device, WGPUQueue queue)
{
    Context ctx;

    {
        ctx.target = create_texture(device, "surface_texture", 1000, 1000, WGPUTextureFormat_BGRA8UnormSrgb, WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_All);
        ctx.depth = create_texture(device, "depth_texture", 1000, 1000, WGPUTextureFormat_Depth24Plus, WGPUTextureUsage_RenderAttachment, WGPUTextureAspect_DepthOnly);

        Texture gradient_texture =  create_texture(device, "gradient_texture", 255, 1, WGPUTextureFormat_R32Float, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureAspect_All);
        ctx.gradient = create_bindable_texture(device, gradient_texture, WGPUShaderStage_Vertex);
        Texture color_texture =  create_texture(device, "color_texture", 255, 1, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, WGPUTextureAspect_All);
        ctx.color = create_bindable_texture(device, color_texture, WGPUShaderStage_Fragment);

        ctx.sampler.layout = wgpuDeviceCreateBindGroupLayout(device, &(WGPUBindGroupLayoutDescriptor)
            {
                .entryCount = 1,
                .entries = &(WGPUBindGroupLayoutEntry) {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex,
                    .sampler = {
                        .type = WGPUSamplerBindingType_NonFiltering
                    }
                }
            });

        ctx.sampler.sampler = wgpuDeviceCreateSampler(device, &(WGPUSamplerDescriptor){
                .addressModeU = WGPUAddressMode_ClampToEdge,
                .addressModeV = WGPUAddressMode_ClampToEdge,
                .addressModeW = WGPUAddressMode_ClampToEdge,
                .magFilter = WGPUFilterMode_Nearest,
                .minFilter = WGPUFilterMode_Nearest,
                .mipmapFilter = WGPUMipmapFilterMode_Nearest,
                .maxAnisotropy = 1
            });

        ctx.sampler.group = wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor){
                .layout = ctx.sampler.layout,
                .entryCount = 1,
                .entries = (WGPUBindGroupEntry[]){
                    [0] = {
                        .binding = 0,
                        .sampler = ctx.sampler.sampler
                    }
                }
            });
    }

    {
        ctx.shader_data.layout = wgpuDeviceCreateBindGroupLayout(device, &(WGPUBindGroupLayoutDescriptor){
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

        ctx.shader_data.buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor){
            .size = sizeof(ShaderData),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

        ctx.shader_data.group = wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor){
            .layout = ctx.shader_data.layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .buffer = ctx.shader_data.buffer,
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
                .bindGroupLayoutCount = 4,
                .bindGroupLayouts = (WGPUBindGroupLayout[]) {
                    [0] = ctx.shader_data.layout,
                    [1] = ctx.sampler.layout,
                    [2] = ctx.gradient.layout,
                    [3] = ctx.color.layout
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

        ctx.n_vertices = array_len(icosahedron_indices) / 3;
        ctx.n_indices = array_len(icosahedron_indices);
        Vertex vertices[ctx.n_vertices * 6 * 6];
        u16 indices[ctx.n_indices * 4 * 4];
        buf_copy(vertices, icosahedron_vertices, sizeof(icosahedron_vertices));
        buf_copy(indices, icosahedron_indices, sizeof(icosahedron_indices));
        subdivide(vertices, &ctx.n_vertices, indices, &ctx.n_indices);
        subdivide(vertices, &ctx.n_vertices, indices, &ctx.n_indices);

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

void text(s8 text)
{
    i32 width, height; ripple_measure_text(text, font_size, &width, &height);
    RIPPLE( FORM( .width = PIXELS(width), .height = PIXELS(height) ), WORDS( .text = text, .color = light ));
}

bool button(s8 label)
{
    bool* open = nullptr;
    RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = PIXELS(font_size) ), RECTANGLE( .color = STATE().is_weak_held ? accent : dark2 ) )
    {
        open = &STATE_USER(bool);
        if (STATE().released)
        {
            debug("open {}", *open);
            STATE_USER(bool) = !*open;
        }
        text(label);
    }
    return *open;
}

bool color_selector(HSV* color)
{
    bool is_interacted = false;

    RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .direction = cld_HORIZONTAL ) )
    {
        u32 full_color = hsv_to_rgb((HSV){ color->hue, 1.0f, 1.0f });
        RIPPLE( FORM( .width = PIXELS(300), .height = PIXELS(300) ),
                RECTANGLE( .color1 = {0x0}, .color2 = {0x0}, .color3 = {0xffffff}, .color4 = RIPPLE_RGB(full_color) ) )
        {
            if (STATE().is_held)
            {
                i32 x = SHAPE().x;
                i32 y = SHAPE().y;
                i32 w = SHAPE().w;
                i32 h = SHAPE().h;

                color->saturation = clamp((f32)(CURSOR().x - x) / w, 0.0f, 1.0f);
                color->value = clamp(1.0f - ((f32)(CURSOR().y - y) / h), 0.0f, 1.0f);
            }

            is_interacted |= STATE().hovered;

            RIPPLE( FORM( .width = PIXELS(0), .height = PIXELS(0), .x = RELATIVE(color->saturation, SVT_RELATIVE_PARENT), .y = RELATIVE(1.0f - color->value, SVT_RELATIVE_PARENT) ) )
            {
                RIPPLE( FORM( .width = PIXELS(10), .height = PIXELS(10), .x = PIXELS(-5), .y = PIXELS(-5)), RECTANGLE( .color = accent ) );
            }
        }

        RIPPLE( FORM( .width = PIXELS(16), .height = RELATIVE(1.0f, SVT_RELATIVE_PARENT) ) )
        {
            i32 y = SHAPE().y;
            i32 h = SHAPE().h;

            if (STATE().is_held)
            {
                color->hue = clamp((f32)(CURSOR().y - y) / h, 0.0f, 1.0f) * 360.0f;
            }

            is_interacted |= STATE().hovered;

            const u32 n = 6;
            for (u32 i = 0; i < n; i++)
            {
                u32 color1 = hsv_to_rgb((HSV){((f32)i/n) * 360.0f, 1.0f, 1.0f});
                u32 color2 = hsv_to_rgb((HSV){((f32)(i+1)/n) * 360.0f, 1.0f, 1.0f});
                RIPPLE( FORM( .height = RELATIVE(1.0f / n, SVT_RELATIVE_PARENT) ),
                        RECTANGLE(  .color1 = RIPPLE_RGB(color2),
                                    .color2 = RIPPLE_RGB(color2),
                                    .color3 = RIPPLE_RGB(color1),
                                    .color4 = RIPPLE_RGB(color1)
                ));
            }

            RIPPLE( FORM( .width = PIXELS(10), .height = PIXELS(10), .x = PIXELS(3), .y = PIXELS((color->hue / 360.0f) * h - 5)),
                    RECTANGLE( .color = accent ) );

        }
    }

    return is_interacted;
}

void color_picker(s8 label, HSV* color)
{
    bool* open = nullptr;

    RIPPLE( FORM( .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD) ) )
    {
        open = &STATE_USER(bool);

        RIPPLE( FORM( .direction = cld_HORIZONTAL, .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD) ) )
        {
            RIPPLE( FORM( .width = PIXELS(font_size) ), RECTANGLE( .color = RIPPLE_RGB(hsv_to_rgb(*color)) ) )
            {
                if (STATE().released)
                {
                    debug("STATE().released: {}", (bool)STATE().released);
                    *open = !*open;
                }
            }

            text(label);
        }

        if (*open)
        {
            /* RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .x = PIXELS(0), .y = RELATIVE(-1.0f, SVT_RELATIVE_CHILD)) ) */
            RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .x = PIXELS(font_size * 0.5f) ) )
            {
                if (color_selector(color)) {
                    *open = true;
                }
            }
        }
    }
}

#define STOP_RAMP_BODY(lerp, to_rgb, selector)\
bool changed = false;\
u32 stop_w = 10;\
u32 sorted[n_stops];\
array_get_sorted_indices(sorted, stops, n_stops, a->t < b->t);\
RIPPLE( FORM( .width = PIXELS(300), .height = PIXELS(32), .direction = cld_HORIZONTAL)) {\
    u32 x = SHAPE().x;\
    u32 w = SHAPE().w;\
    f32 t = 0.0f;\
    for (i32 i = 0; i < (i32)n_stops; i++) {\
        u32 color = to_rgb(stops[sorted[i]].value);\
        u32 prev_color = to_rgb(stops[sorted[max(i - 1, 0)]].value);\
        RIPPLE( FORM( .x = RELATIVE(t, SVT_RELATIVE_PARENT),\
                      .width = RELATIVE(stops[sorted[i]].t - t + 3.0f / 300.0f, SVT_RELATIVE_PARENT)), \
            RECTANGLE(\
                .color1 = {prev_color}, .color3 = {prev_color},\
                .color2 = {color},      .color4 = {color},\
            ) );\
        t = stops[sorted[i]].t;\
    }\
    RIPPLE( FORM( .x = RELATIVE(stops[sorted[n_stops - 1]].t, SVT_RELATIVE_PARENT), .width = RELATIVE(1.0f - stops[sorted[n_stops - 1]].t, SVT_RELATIVE_PARENT)),\
            RECTANGLE( .color = {value_to_rgb(stops[sorted[n_stops - 1]].value)} )\
    );\
    changed = STATE().first_render;\
    CENTERED_VERTICAL(\
    RIPPLE( FORM( .direction = cld_HORIZONTAL ) ) {\
        for (u32 i = 0; i < n_stops; i++) {\
            RIPPLE( FORM( .x = PIXELS(stops[i].t * (w - stop_w)), .width = PIXELS(stop_w), .height = PIXELS(10) ), RECTANGLE( .color = accent ) ) {\
                if (!changed && STATE().is_held) {\
                    stops[i].t = clamp(((f32)CURSOR().x - (f32)x) / (f32)w, 0.0f, 1.0f);\
                    changed = true;\
                }\
            }\
        }\
    }\
    );\
}\
if (!changed) return false;\
u32 buffer_i = 0;\
for (u32 i = 0; i < n_stops + 1; i++)\
{\
    typeof(*stops) v0 = i == 0 ? (typeof(*stops)){ 0.0f, stops[sorted[0]].value } : stops[sorted[i - 1]];\
    typeof(*stops) v1 = i == n_stops ? (typeof(*stops)){ 1.0f, stops[sorted[i - 1]].value } : stops[sorted[i]];\
    i32 n_steps = ceil((f32)buffer_len * (v1.t - v0.t));\
    for (i32 j = 0; j < n_steps; j++)\
    {\
        f32 t = (f32)j / (f32)n_steps;\
        buffer[buffer_i++] = lerp;\
    }\
}\
return true

typedef struct {
    f32 t;
    f32 value;
} FloatRampStop;
bool float_ramp(FloatRampStop* stops, u32 n_stops, f32* buffer, u32 buffer_len)
{
    STOP_RAMP_BODY(v0.value * (1.0f - t) + v1.value * t, value_to_rgb, (void));
}

u32 lerp_color(u32 a, u32 b, f32 t)
{
    t = clamp(t, 0.0f, 1.0f);
    f32 t2 = 1.0f - t;

    f32 ar = (f32)((a >> 16) & 0xFF);
    f32 ag = (f32)((a >>  8) & 0xFF);
    f32 ab = (f32)(a & 0xFF);
    f32 br = (f32)((b >> 16) & 0xFF);
    f32 bg = (f32)((b >>  8) & 0xFF);
    f32 bb = (f32)(b & 0xFF);

    uint32_t r = min((uint32_t)(ar * t2 + br * t), 0xff);
    uint32_t g = min((uint32_t)(ag * t2 + bg * t), 0xff);
    uint32_t bl= min((uint32_t)(ab * t2 + bb * t), 0xff);

    return (0xff << 24) | (bl << 16) | (g << 8) | r;
}

typedef struct {
    f32 t;
    u32 value;
} ColorRampStop;
bool color_ramp(ColorRampStop* stops, u32 n_stops, u32* buffer, u32 buffer_len)
{
    STOP_RAMP_BODY(lerp_color(v0.value, v1.value, t), (u32), color_selector);
    return false;
}

void slider(const char* label, f32* value, f32 max, f32 min, Allocator* str_allocator)
{
    RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = PIXELS(font_size), .direction = cld_HORIZONTAL ) )
    {
        RIPPLE( FORM( .width = PIXELS(315), .direction = cld_HORIZONTAL ))
        {
            f32 range = max - min;
            f32 t = clamp(( *value - min ) / range, 0.0f, 1.0f);
            u32 w = SHAPE().w;
            u32 x = SHAPE().x;

            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS((1.0f - t) * (w - 15)) , .height = PIXELS(font_size * 0.5f)), RECTANGLE( .color = dark2 ) );
            );

            RIPPLE( FORM( .width = PIXELS(15)), RECTANGLE( .color = accent ) )
            {
                if (STATE().is_held)
                {
                    *value = (1.0f - (clamp((f32)CURSOR().x - (f32)x, 0.0f, (f32)w) / (f32)w)) * range + min;
                }
            }

            CENTERED_VERTICAL(
                RIPPLE( FORM( .width = PIXELS(t * (w - 15)), .height = PIXELS(font_size * 0.5f)), RECTANGLE( .color = dark2 ) );
            );
        }

        RIPPLE( FORM( .width = PIXELS(5) ) );

        text(marrow_format("{}: {.2f}", str_allocator, label, *value));
    }
}

int main(int argc, char* argv[])
{
    RippleBackendRendererConfig config = ripple_backend_renderer_default_config();
    ripple_initialize(ripple_backend_window_default_config(), config);

    WGPUQueue queue = wgpuDeviceGetQueue(config.device);

    Context ctx = create_context(config.device, queue);
    (void) ctx;

    bool main_is_open = true;

    f32 prev_time = 0.0f;
    f32 dt_accum = 0.0f;
    f32 dt_samples = 0.0f;

    BumpAllocator str_allocator = bump_allocator_create();

    ShaderData shader_data = {
        .gain = 1.2f,
        .lacunarity = 5.0f,
        .time_scale = 1.0f,
        .speed = 8.0f,
        .offset = 0.24f,
        .height = 8.35f,
        .scale = 0.13f
    };

    FloatRampStop gradient_stops[] = {
        [0] = (FloatRampStop){ 0.075f, 0.0f },
        [1] = (FloatRampStop){ 0.5f, 0.5f },
        [2] = (FloatRampStop){ 0.7, 1.0f },
    };

    ColorRampStop color_stops[] = {
        [0] = (ColorRampStop){ 0.6f, 0xff0000 },
        [1] = (ColorRampStop){ 0.65f, 0xff7400 },
        [2] = (ColorRampStop){ 0.8, 0xfffb00 },
        [3] = (ColorRampStop){ 1.0f, 0xffffff },
    };

    bool debug_window_open = true;

    f32 zoom = 5.0f;

    RippleColor* original_colors[] = { &dark, &dark2, &accent, &light };
    HSV colors[] = { rgb_to_hsv(dark.value), rgb_to_hsv(dark2.value), rgb_to_hsv(accent.value), rgb_to_hsv(light.value) };

    while (main_is_open) {
        f32 time = shader_data.time = glfwGetTime();//(f32)SDL_GetTicks() / 1000.0f;
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
            shader_data.camera_position = (vec3s){ .x = sin(time) * 40.0f, .y = 40.0f, .z = cos(time) * 40.0f};

            mat4s proj = glms_ortho(-zoom, +zoom, -zoom, +zoom, 0.1f, 1000.0f);
            mat4s view = glms_lookat( shader_data.camera_position, (vec3s){ .x = 0.0f, .y = 2.0f, .z = 0.0f}, GLMS_YUP );
            glm_mat4_copy(glms_mat4_mul(proj, view).raw, shader_data.camera_matrix); // cam.raw is plain mat4
        }

        SURFACE( .title = S8("surface"), .width = 800, .height = 800, .clear_color = dark, .is_open = &main_is_open )
        {
            if (!debug_window_open)
            {
                RIPPLE( FORM( .width = RELATIVE(1.0f, SVT_RELATIVE_CHILD), .height = RELATIVE(1.0f, SVT_RELATIVE_CHILD) ), RECTANGLE( .color = STATE().hovered ? dark2 : dark ))
                {
                    debug_window_open = STATE().released;
                    text(S8("text"));
                }
            }

            RIPPLE( IMAGE( ctx.target.view ) );
        }

        if (debug_window_open)
        SURFACE( .title = S8("debug"), .width = 400, .height = 400, .clear_color = dark, .is_open = &debug_window_open )
        RIPPLE( RECTANGLE( .color = dark ) )
        {
            text(marrow_format("fps rn is: {.2f}", &str_allocator, 1.0f / (dt_accum / dt_samples)));

            if (button(S8("fire settings")))
            {
                slider("gain", &shader_data.gain, 0.0f, 3.0f, (Allocator*)&str_allocator);
                slider("speed", &shader_data.speed, 0.0f, 60.0f, (Allocator*)&str_allocator);
                slider("offset", &shader_data.offset, 0.0f, 5.0f, (Allocator*)&str_allocator);
                slider("height", &shader_data.height, 0.0f, 10.0f, (Allocator*)&str_allocator);
                slider("time_scale", &shader_data.time_scale, 0.0f, 1.0f, (Allocator*)&str_allocator);
                slider("lacunarity", &shader_data.lacunarity, 0.0f, 5.0f, (Allocator*)&str_allocator);
                slider("scale", &shader_data.scale, 0.0f, 10.0f, (Allocator*)&str_allocator);
                slider("zoom", &zoom, 1.0f, 10.0f, (Allocator*)&str_allocator);

                f32 float_buffer[256];
                if (float_ramp(gradient_stops, array_len(gradient_stops), float_buffer, array_len(float_buffer)))
                {
                    wgpuQueueWriteTexture(queue, &(WGPUImageCopyTexture){
                            .texture = ctx.gradient.texture.texture,
                            .aspect = WGPUTextureAspect_All
                        },
                        float_buffer,
                        sizeof(float_buffer),
                        &(WGPUTextureDataLayout){
                            .bytesPerRow = sizeof(float_buffer),
                            .rowsPerImage = 1
                        },
                        &ctx.gradient.texture.extent
                    );
                }

                u32 color_buffer[256];
                if (color_ramp(color_stops, array_len(color_stops), color_buffer, array_len(color_buffer)))
                {
                    wgpuQueueWriteTexture(queue, &(WGPUImageCopyTexture){
                            .texture = ctx.color.texture.texture,
                            .aspect = WGPUTextureAspect_All
                        },
                        color_buffer,
                        sizeof(color_buffer),
                        &(WGPUTextureDataLayout){
                            .bytesPerRow = sizeof(color_buffer),
                            .rowsPerImage = 1
                        },
                        &ctx.color.texture.extent
                    );
                }
            }

            for_each_i(color, colors, i)
            {
                color_picker(marrow_format(": (0x{XD})", (Allocator*)&str_allocator, hsv_to_rgb(*color)), color);
                original_colors[i]->value = hsv_to_rgb(*color);
            }
        }

        wgpuQueueWriteBuffer(queue, ctx.shader_data.buffer, 0, &shader_data, sizeof(shader_data));

        RippleRenderData render_data = RIPPLE_RENDER_BEGIN();

        {
            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
                    .colorAttachmentCount = 1,
                    .colorAttachments = &(WGPURenderPassColorAttachment){
                        .view = ctx.target.view,
                        .loadOp = WGPULoadOp_Clear,
                        .storeOp = WGPUStoreOp_Store,
                        .clearValue = (WGPUColor){ 0.09f, 0.192f, 0.243f, 1.0f },
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
            wgpuRenderPassEncoderSetBindGroup(render_pass, 0, ctx.shader_data.group, 0, NULL);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 1, ctx.sampler.group, 0, NULL);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 2, ctx.gradient.group, 0, NULL);
            wgpuRenderPassEncoderSetBindGroup(render_pass, 3, ctx.color.group, 0, NULL);
            wgpuRenderPassEncoderDrawIndexed(render_pass, ctx.n_indices, array_len(instances), 0, 0, 0);

            wgpuRenderPassEncoderEnd(render_pass);
        }

        RIPPLE_RENDER_END(render_data);

        bump_allocator_reset(&str_allocator);
    }

    return 0;
}
