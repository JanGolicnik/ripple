#ifndef RIPPLE_WGPU_H
#define RIPPLE_WGPU_H

#if (RIPPLE_BACKEND) & RIPPLE_GLFW
#include <glfw3webgpu.h>
#endif // RIPPLE_GLFW
#if (RIPPLE_BACKEND) & RIPPLE_SDL
#include <sdl2webgpu.h>
#endif // RIPPLE_SDL

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb/stb_truetype.h>

#include <math.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_WGPU_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

#define WEBGPU_STR_EXACT(str) (WGPUStringView) { .data = str, .length = sizeof(str) - 1 }
#define WEBGPU_STR(str) (WGPUStringView) { .data = str, .length = WGPU_STRLEN }

typedef struct {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
} RippleBackendRendererConfig;

typedef struct {
    WGPUCommandEncoder encoder;
} RippleRenderData;

typedef WGPUTextureView RippleImage;

const f32 FONT_SIZE = 128.0f;
const u32 BITMAP_SIZE = 1024;

#ifdef RIPPLE_WGPU_IMPLEMENTATION

typedef struct {
    WGPUAdapter adapter;
    bool request_done;
} RequestAdapterUserData;

static void request_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2)
{
    if (status != WGPURequestAdapterStatus_Success)
        abort("Failed to get WebGPU adapter");

    RequestAdapterUserData* callback_user_data = userdata1;
    callback_user_data->adapter = adapter;
    callback_user_data->request_done = true;
}

static WGPUAdapter get_adapter(WGPUInstance instance, WGPURequestAdapterOptions request_adapter_options)
{
    RequestAdapterUserData request_adapter_user_data = { 0 };
    wgpuInstanceRequestAdapter(instance, &request_adapter_options, (WGPURequestAdapterCallbackInfo) {
        .mode = WGPUCallbackMode_AllowSpontaneous,
        .callback = request_adapter_callback,
        .userdata1 = &request_adapter_user_data
    });
    return request_adapter_user_data.adapter;
}

typedef struct {
    WGPUDevice device;
    bool request_done;
} RequestDeviceUserData;

static void request_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2)
{
    if (status != WGPURequestDeviceStatus_Success)
    {
        abort("Failed to get WebGPU device");
    }

    RequestDeviceUserData* callback_user_data = userdata1;
    callback_user_data->device = device;
    callback_user_data->request_done = true;
}

static void device_uncaptured_error_callback(WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2)
{
    if (message.length == WGPU_STRLEN)
    {
        error("Uncaptured device error ({}): {}", (u32)type, message.data);
    }
    else
    {
        char data[message.length + 1];
        buf_copy(data, message.data, message.length + 1);
        data[message.length] = 0;
        error("Uncaptured device error ({}): {}", (u32)type, data);
    }
}

static WGPUDevice get_device(WGPUAdapter adapter)
{
    WGPUDeviceDescriptor device_descriptor = {
        .label = WEBGPU_STR("Device :D"),
        .defaultQueue.label = WEBGPU_STR("Default queue"),
        .uncapturedErrorCallbackInfo = (WGPUUncapturedErrorCallbackInfo) {
            .callback = device_uncaptured_error_callback,
        }
    };
    RequestDeviceUserData request_device_user_data = { 0 };
    wgpuAdapterRequestDevice(
        adapter,
        &device_descriptor,
        (WGPURequestDeviceCallbackInfo){
            .mode = WGPUCallbackMode_AllowSpontaneous,
            .callback = request_device_callback,
            .userdata1 = &request_device_user_data
        }
    );

    WGPUDevice device = request_device_user_data.device;
    return device;
}

RippleBackendRendererConfig ripple_backend_renderer_default_config()
{
    RippleBackendRendererConfig config = { 0 };

    config.instance = wgpuCreateInstance(&(WGPUInstanceDescriptor) { 0 });
    if (!config.instance)
        abort("Failed to create WebGPU instance.");
    debug("Successfully created the WebGPU instance!");

    config.adapter = get_adapter(config.instance, (WGPURequestAdapterOptions){
        .powerPreference = WGPUPowerPreference_HighPerformance
    });
    if (!config.adapter)
        abort("Failed to get the adapter!");
    debug("Successfully got the adapter!");

    config.device = get_device(config.adapter);
    if (!config.device)
        abort("Failed to get the device!");
    debug("Succesfully got the device!");

    return config;
}

static bool configure_surface(WGPUDevice device, WGPUSurface surface, WGPUTextureFormat format, u32 width, u32 height)
{
    wgpuSurfaceUnconfigure(surface);
    if (width == 0 || height == 0)
        return false;

    wgpuSurfaceConfigure(surface, &(WGPUSurfaceConfiguration){
            .width = width,
            .height = height,
            .format = format,
            .usage = WGPUTextureUsage_RenderAttachment,
            .device = device,
            .presentMode = WGPUPresentMode_Fifo,
            .alphaMode = WGPUCompositeAlphaMode_Auto
        });
    return true;
}

static f32 vertex_data[] = {
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    0.0, 1.0,
};
static const u32 vertex_data_size = 2 * sizeof(vertex_data[0]);
static const u32 vertex_count = sizeof(vertex_data) / vertex_data_size;

static uint16_t index_data[] = {
    0, 1, 2,
    0, 2, 3
};
static const u32 index_data_size = 1 * sizeof(index_data[0]);
static const u32 index_count = sizeof(index_data) / index_data_size;

typedef struct {
    f32 pos[2];
    f32 size[2];
    f32 uv[4];
    f32 color1[4];
    f32 color2[4];
    f32 color3[4];
    f32 color4[4];
    f32 color_padding[1];
    u32 image_index;
    u32 image_index_padding[3];
} RippleWGPUInstance;

const char* shader =
"struct ShaderData {\n"
"    color: vec4f,\n"
"    resolution: vec2i,\n"
"    time: f32,\n"
"}\n"
"@group(0) @binding(0) var<uniform> shader_data: ShaderData;\n"
"\n"
"@group(1) @binding(0) var texture1: texture_2d<f32>;\n"
"@group(1) @binding(1) var texture2: texture_2d<f32>;\n"
"@group(1) @binding(2) var texture3: texture_2d<f32>;\n"
"@group(1) @binding(3) var texture4: texture_2d<f32>;\n"
"@group(1) @binding(4) var texture5: texture_2d<f32>;\n"
"\n"
"@group(2) @binding(0) var texture_sampler: sampler;\n"
"\n"
"struct VertexInput {\n"
"    @location(0) position: vec2f,\n"
"}\n"
"\n"
"struct InstanceInput {\n"
"    @location(1) position: vec2f,\n"
"    @location(2) size: vec2f,\n"
"    @location(3) uv: vec4f,\n"
"    @location(4) color1: vec4f,\n"
"    @location(5) color2: vec4f,\n"
"    @location(6) color3: vec4f,\n"
"    @location(7) color4: vec4f,\n"
"    @location(8) image_index: u32,\n"
"}\n"
"\n"
"struct VertexOutput{\n"
"    @builtin(position) position: vec4f,\n"
"    @location(0) uv: vec2f,\n"
"    @interpolate(flat) @location(1) image_index: u32,\n"
"    @location(2) color1: vec4f,\n"
"    @location(3) color2: vec4f,\n"
"    @location(4) color3: vec4f,\n"
"    @location(5) color4: vec4f,\n"
"}\n"
"\n"
"@vertex \n"
"fn vs_main(v: VertexInput, i: InstanceInput, @builtin(vertex_index) index: u32) -> VertexOutput {\n"
"    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));\n"
"    var position = i.position + v.position * i.size;\n"
"    position = position / resolution;\n"
"    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2<f32>(1.0f, 1.0f);\n"
"    var uv = i.uv.xy;\n"
"    if (index == 1) { uv = i.uv.zy; }\n"
"    else if (index == 2) { uv = i.uv.zw; }\n"
"    else if (index == 3) { uv = i.uv.xw; }\n"
"    \n"
"    var out: VertexOutput;\n"
"    out.position = vec4f(position.x, position.y, 0.0, 1.0);\n"
"    out.uv = uv;\n"
"    out.color1 = i.color1;\n"
"    out.color2 = i.color2;\n"
"    out.color3 = i.color3;\n"
"    out.color4 = i.color4;\n"
"    out.image_index = i.image_index;\n"
"    return out;\n"
"}\n"
"\n"
"@fragment \n"
"fn fs_main(in: VertexOutput) -> @location(0) vec4f {\n"
"    var tex_color1 = textureSample(texture1, texture_sampler, in.uv);\n"
"    var tex_color2 = vec4f(textureSample(texture2, texture_sampler, in.uv).r);\n"
"    var tex_color3 = textureSample(texture3, texture_sampler, in.uv);\n"
"    var tex_color4 = textureSample(texture4, texture_sampler, in.uv);\n"
"    var tex_color5 = textureSample(texture5, texture_sampler, in.uv);\n"
"    var tex_color: vec4f;\n"
"    switch in.image_index {\n"
"        default { tex_color = tex_color1; }\n"
"        case 0u { tex_color = tex_color1; }\n"
"        case 1u { tex_color = tex_color2; }\n"
"        case 2u { tex_color = tex_color3; }\n"
"        case 3u { tex_color = tex_color4; }\n"
"        case 4u { tex_color = tex_color5; }\n"
"    }\n"
"    let gradient = mix(mix(in.color3, in.color4, in.uv.x), mix(in.color1, in.color2, in.uv.x), in.uv.y);\n"
"    let color = tex_color * gradient;\n"
"    let linear_color = pow(color.rgb, vec3f(2.2));\n"
"    return vec4f(linear_color, color.a);\n"
"}\n";

typedef struct {
    f32 color[4];
    i32 resolution[2];
    f32 time;
    f32 _padding[1];
} RippleWGPUShaderData;

typedef struct {
    RippleImage images[5];
    u32 n_images;
    u32 instance_index;
} _RippleImageInstancePair;

typedef struct RippleBackendWindowRenderer {
    WGPUSurface surface;
    WGPUTextureFormat surface_format;

    // these two are only set inbetween render_window_end and render_end
    WGPUSurfaceTexture surface_texture;
    WGPUTextureView surface_texture_view;

    WGPUBuffer instance_buffer;
    WGPUBuffer uniform_buffer;
    WGPUBindGroup bind_group;

    RippleWGPUShaderData shader_data;

    u32 instance_buffer_size;
    VEKTOR(RippleWGPUInstance) instances;

    VEKTOR( _RippleImageInstancePair ) images;

    bool surface_is_configured;
} RippleBackendWindowRenderer;

struct {
    RippleBackendRendererConfig config;
    WGPUQueue queue;

    WGPUBindGroupLayout bind_group_layout;
    WGPUBindGroupLayout image_bind_group_layout;
    WGPUPipelineLayout pipeline_layout;
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;

    WGPUSampler sampler;
    WGPUBindGroupLayout sampler_bind_group_layout;
    WGPUBindGroup sampler_bind_group;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
    } white_pixel;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        stbtt_bakedchar glyphs[96];
    } font;
} _context;

void ripple_backend_renderer_initialize(RippleBackendRendererConfig config)
{
    debug("INITIALIZNG BACKEND");

    _context.config = config;
    _context.queue = wgpuDeviceGetQueue(_context.config.device);

    // create font
    {
        // load data
        FILE* file = fopen("./roboto.ttf", "rb");
        if ( !file )
            abort("Could not load font file :(");
        fseek(file, 0, SEEK_END);
        const usize file_size = ftell(file);
        rewind(file);
        u8 file_buffer[file_size];
        buf_set(file_buffer, 0, file_size);
        fread(file_buffer, 1, file_size, file);
        fclose(file);

        // bake bitmap
        u8 bitmap_buffer[BITMAP_SIZE * BITMAP_SIZE];
        if (stbtt_BakeFontBitmap(file_buffer, 0, FONT_SIZE, bitmap_buffer, BITMAP_SIZE, BITMAP_SIZE, 32, 96, _context.font.glyphs) == 0)
        {
            abort("failed baking bitmap");
        }

        _context.font.texture = wgpuDeviceCreateTexture(_context.config.device, &(WGPUTextureDescriptor){
                .label = WEBGPU_STR("fontAtlas"),
                .size = (WGPUExtent3D){ .width = BITMAP_SIZE, .height = BITMAP_SIZE, .depthOrArrayLayers = 1 },
                .format = WGPUTextureFormat_R8Unorm,
                .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        wgpuQueueWriteTexture(_context.queue, &(WGPUTexelCopyTextureInfo){
                .texture = _context.font.texture,
                .aspect = WGPUTextureAspect_All
            },
            bitmap_buffer,
            BITMAP_SIZE * BITMAP_SIZE,
            &(WGPUTexelCopyBufferLayout){
                .bytesPerRow = BITMAP_SIZE,
                .rowsPerImage = BITMAP_SIZE
            },
            &(WGPUExtent3D){
                .width = BITMAP_SIZE,
                .height = BITMAP_SIZE,
                .depthOrArrayLayers = 1
            }
        );

        _context.font.view = wgpuTextureCreateView(_context.font.texture, &(WGPUTextureViewDescriptor){
                .format = WGPUTextureFormat_R8Unorm,
                .dimension = WGPUTextureViewDimension_2D,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
                .aspect = WGPUTextureAspect_All,
            });
    }

    {
        _context.white_pixel.texture = wgpuDeviceCreateTexture(_context.config.device, &(WGPUTextureDescriptor){
                .label = WEBGPU_STR("white_pixel"),
                .size = (WGPUExtent3D){ .width = 1, .height = 1, .depthOrArrayLayers = 1 },
                .format = WGPUTextureFormat_RGBA8Unorm,
                .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        _context.white_pixel.view = wgpuTextureCreateView(_context.white_pixel.texture, &(WGPUTextureViewDescriptor){
                .format = WGPUTextureFormat_RGBA8Unorm,
                .dimension = WGPUTextureViewDimension_2D,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
                .aspect = WGPUTextureAspect_All,
            });

        u32 white = 0xffffffff;
        wgpuQueueWriteTexture(_context.queue, &(WGPUTexelCopyTextureInfo){
                .texture = _context.white_pixel.texture,
                .aspect = WGPUTextureAspect_All
            },
            &white,
            4,
            &(WGPUTexelCopyBufferLayout){
                .bytesPerRow = 4,
                .rowsPerImage = 1
            },
            &(WGPUExtent3D){
                .width = 1,
                .height = 1,
                .depthOrArrayLayers = 1
            }
        );
    }

    _context.bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.config.device, &(WGPUBindGroupLayoutDescriptor){
            .entryCount = 1,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                    .buffer = {
                        .type = WGPUBufferBindingType_Uniform,
                        .minBindingSize = sizeof(RippleWGPUShaderData)
                    }
                }
            }
        });

    _context.image_bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.config.device, &(WGPUBindGroupLayoutDescriptor)
        {
            .entryCount = 5,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [1] = {
                    .binding = 1,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [2] = {
                    .binding = 2,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [3] = {
                    .binding = 3,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [4] = {
                    .binding = 4,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_UnfilterableFloat,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                }
            }
        });

    _context.sampler_bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.config.device, &(WGPUBindGroupLayoutDescriptor)
        {
            .entryCount = 1,
            .entries = &(WGPUBindGroupLayoutEntry) {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment,
                .sampler = {
                    .type = WGPUSamplerBindingType_NonFiltering
                }
            }
        });

    _context.sampler = wgpuDeviceCreateSampler(_context.config.device, &(WGPUSamplerDescriptor){
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .addressModeW = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Nearest,
            .minFilter = WGPUFilterMode_Nearest,
            .mipmapFilter = WGPUMipmapFilterMode_Nearest,
            .maxAnisotropy = 1
        });

    _context.sampler_bind_group = wgpuDeviceCreateBindGroup(_context.config.device, &(WGPUBindGroupDescriptor){
            .layout = _context.sampler_bind_group_layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .sampler = _context.sampler
                }
            }
        });

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(_context.config.device, &(WGPUShaderModuleDescriptor){
            .label = WEBGPU_STR("Shdader descriptor"),
            .nextInChain = &((WGPUShaderSourceWGSL) {
                .chain.sType = WGPUSType_ShaderSourceWGSL,
                .code = WEBGPU_STR(shader)
            }).chain
        });

    _context.pipeline_layout = wgpuDeviceCreatePipelineLayout(_context.config.device, &(WGPUPipelineLayoutDescriptor){
            .bindGroupLayoutCount = 3,
            .bindGroupLayouts = (WGPUBindGroupLayout[]) {
                [0] = _context.bind_group_layout,
                [1] = _context.image_bind_group_layout,
                [2] = _context.sampler_bind_group_layout,
            }
        });

    _context.pipeline = wgpuDeviceCreateRenderPipeline(_context.config.device, &(WGPURenderPipelineDescriptor){
            .vertex = {
                .module = shader_module,
                .entryPoint = WEBGPU_STR("vs_main"),
                .bufferCount = 2,
                .buffers = (WGPUVertexBufferLayout[]) {
                    [0] = {
                        .attributeCount = 1,
                        .attributes = (WGPUVertexAttribute[]) {
                            [0] = {
                                .shaderLocation = 0,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = 0
                            }
                        },
                        .arrayStride = vertex_data_size,
                        .stepMode = WGPUVertexStepMode_Vertex,
                    },
                    [1] = {
                        .attributeCount = 8,
                        .attributes = (WGPUVertexAttribute[]) {
                            [0] = {
                                .shaderLocation = 1,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = 0
                            },
                            [1] = {
                                .shaderLocation = 2,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = offsetof(RippleWGPUInstance, size)
                            },
                            [2] = {
                                .shaderLocation = 3,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(RippleWGPUInstance, uv)
                            },
                            [3] = {
                                .shaderLocation = 4,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(RippleWGPUInstance, color1)
                            },
                            [4] = {
                                .shaderLocation = 5,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(RippleWGPUInstance, color2)
                            },
                            [5] = {
                                .shaderLocation = 6,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(RippleWGPUInstance, color3)
                            },
                            [6] = {
                                .shaderLocation = 7,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(RippleWGPUInstance, color4)
                            },
                            [7] = {
                                .shaderLocation = 8,
                                .format = WGPUVertexFormat_Uint32,
                                .offset = offsetof(RippleWGPUInstance, image_index)
                            }
                        },
                        .arrayStride = sizeof(RippleWGPUInstance),
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
                .entryPoint = WEBGPU_STR("fs_main"),
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
            .layout = _context.pipeline_layout
        });

    wgpuShaderModuleRelease(shader_module);

    _context.vertex_buffer = wgpuDeviceCreateBuffer(_context.config.device, &(WGPUBufferDescriptor) {
           .size = sizeof(vertex_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });
    wgpuQueueWriteBuffer(_context.queue, _context.vertex_buffer, 0, vertex_data, sizeof(vertex_data));

    _context.index_buffer = wgpuDeviceCreateBuffer(_context.config.device, &(WGPUBufferDescriptor) {
           .size = sizeof(index_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
        });
    wgpuQueueWriteBuffer(_context.queue, _context.index_buffer, 0, index_data, sizeof(index_data));
}

RippleBackendWindowRenderer ripple_backend_window_renderer_create(u64 id, RippleWindowConfig config, const RippleBackendWindow* window)
{
    RippleBackendWindowRenderer renderer = { 0 };
    renderer.surface =
#if (RIPPLE_BACKEND) & RIPPLE_GLFW
        glfwGetWGPUSurface(_context.config.instance, window->window);
#endif // RIPPLE_GLFW
#if (RIPPLE_BACKEND) & RIPPLE_SDL
        SDL_GetWGPUSurface(_context.config.instance, window->window);
#endif // RIPPLE_SDL
    renderer.surface_format = WGPUTextureFormat_BGRA8UnormSrgb;//wgpuSurfaceGetPreferredFormat(renderer.surface, _context.config.adapter);

    renderer.uniform_buffer = wgpuDeviceCreateBuffer(_context.config.device, &(WGPUBufferDescriptor){
            .size = sizeof(renderer.shader_data),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

    renderer.bind_group = wgpuDeviceCreateBindGroup(_context.config.device, &(WGPUBindGroupDescriptor){
            .layout = _context.bind_group_layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .buffer = renderer.uniform_buffer,
                    .offset = 0,
                    .size = sizeof(renderer.shader_data)
                }
            }
        });

    return renderer;
}

RippleRenderData ripple_backend_render_begin()
{
    wgpuInstanceProcessEvents(_context.config.instance);
    return (RippleRenderData){
        .encoder = wgpuDeviceCreateCommandEncoder(_context.config.device, &(WGPUCommandEncoderDescriptor){ .label = WEBGPU_STR("Command encoder")})
    };
}

void ripple_backend_render_window_begin(RippleBackendWindow* window, RippleBackendWindowRenderer* renderer, RippleRenderData render_data)
{
    (void) render_data;

    if (window->resized)
    {
        renderer->shader_data.resolution[0] = window->width;
        renderer->shader_data.resolution[1] = window->height;
        renderer->surface_is_configured = configure_surface(_context.config.device, renderer->surface, renderer->surface_format, renderer->shader_data.resolution[0], renderer->shader_data.resolution[1]);
        window->resized = false;
    }

    renderer->shader_data.time = 0.0f; // TODO
    renderer->shader_data.color[0] = sinf(renderer->shader_data.time) * 0.5f + 0.5f;
    renderer->shader_data.color[1] = sinf(renderer->shader_data.time * 2.5f) * 0.25f + 0.5f;
    renderer->shader_data.color[2] = sinf(renderer->shader_data.time * 5.0f) * 0.1f + 0.5f;
    wgpuQueueWriteBuffer(_context.queue, renderer->uniform_buffer, 0, &renderer->shader_data, sizeof(renderer->shader_data));

    vektor_clear(renderer->instances);
    vektor_clear(renderer->images);
    vektor_add(renderer->images, (_RippleImageInstancePair) {
        .images = { [0] = _context.white_pixel.view, [1] = _context.font.view },
        .n_images = 2,
        .instance_index = 0
    });
}

static void _ripple_backend_color_to_color(RippleColor color, f32 out_color[4])
{
    if (color.format == RCF_RGB)
    {
        out_color[0] = (f32)((color.value >> 16) & 0xff) / 255.0f;
        out_color[1] = (f32)((color.value >> 8) & 0xff) / 255.0f;
        out_color[2] = (f32)(color.value & 0xff) / 255.0f;
        out_color[3] = 1.0f;
    }
    else
    {
        out_color[0] = (f32)((color.value >> 24) & 0xff) / 255.0f;
        out_color[1] = (f32)((color.value >> 16) & 0xff) / 255.0f;
        out_color[2] = (f32)((color.value >> 8) & 0xff) / 255.0f;
        out_color[3] = (f32)(color.value & 0xff) / 255.0f;
    }
}

void ripple_backend_render_window_end(RippleBackendWindowRenderer* window, RippleRenderData render_data, RippleColor clear_color)
{
    if (!window->surface_is_configured)
        return;

    if (!window->instance_buffer ||
        window->instance_buffer_size < window->instances.size)
    {
        window->instance_buffer_size = window->instances.size + 1;

        if (window->instance_buffer)
        {
            wgpuBufferRelease(window->instance_buffer);
        }

        window->instance_buffer =
            wgpuDeviceCreateBuffer(_context.config.device, &(WGPUBufferDescriptor) {
                .size = window->instance_buffer_size * sizeof(*window->instances.items),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            });
    }

    wgpuQueueWriteBuffer(_context.queue,
                         window->instance_buffer,
                         0,
                         window->instances.items,
                         window->instances.n_items * sizeof(*window->instances.items));

    wgpuSurfaceGetCurrentTexture(window->surface, &window->surface_texture);

    window->surface_texture_view = wgpuTextureCreateView(window->surface_texture.texture, &(WGPUTextureViewDescriptor){
            .label = WEBGPU_STR("Surface texture view"),
            .format = wgpuTextureGetFormat(window->surface_texture.texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        });

    f32 clear_color_f32[4]; _ripple_backend_color_to_color(clear_color, clear_color_f32);
    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment){
                .view = window->surface_texture_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = (WGPUColor){ clear_color_f32[0], clear_color_f32[1], clear_color_f32[2], clear_color_f32[3] },
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
            },
            .depthStencilAttachment = nullptr
        });

    wgpuRenderPassEncoderSetPipeline(render_pass, _context.pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, _context.vertex_buffer, 0, wgpuBufferGetSize(_context.vertex_buffer));
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, _context.index_buffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(_context.index_buffer));
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, window->bind_group, 0, nullptr);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 2, _context.sampler_bind_group, 0, nullptr);

    WGPUBindGroup bind_groups[window->images.n_items];

    u32 instance_index = 0;
    for (u32 i = 0; i < window->images.n_items; i++)
    {
        _RippleImageInstancePair* image_pair = &window->images.items[i];
        WGPUBindGroupEntry image_textures[array_len(image_pair->images)];
        for (u32 j = 0; j < array_len(image_pair->images); j++)
        {
            image_textures[j] = (WGPUBindGroupEntry){
                .binding = j,
                .textureView = image_pair->images[min(j, image_pair->n_images - 1)]
            };
        }

        bind_groups[i] = wgpuDeviceCreateBindGroup(_context.config.device, &(WGPUBindGroupDescriptor)
            {
                .layout = _context.image_bind_group_layout,
                .entryCount = array_len(image_textures),
                .entries = image_textures
            });

        u32 n_instances = ((i == window->images.n_items - 1) ? window->instances.n_items : image_pair->instance_index) - instance_index;

        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, window->instance_buffer, 0, n_instances * sizeof(RippleWGPUInstance));
        wgpuRenderPassEncoderSetBindGroup(render_pass, 1, bind_groups[i], 0, nullptr);
        wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, n_instances, 0, 0, instance_index);

        instance_index += n_instances;
    }

    wgpuRenderPassEncoderEnd(render_pass);

    for (u32 i = 0; i < array_len(bind_groups); i++)
    {
        wgpuBindGroupRelease(bind_groups[i]);
    }
}

void ripple_backend_render_end(RippleRenderData render_data)
{
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(render_data.encoder, &(WGPUCommandBufferDescriptor){ .label = WEBGPU_STR("Command buffer") });
    wgpuCommandEncoderRelease(render_data.encoder);

    wgpuQueueSubmit(_context.queue, 1, &command);
    wgpuCommandBufferRelease(command);
}

void ripple_backend_window_present(RippleBackendWindowRenderer* renderer)
{
    if (renderer->surface_is_configured)
    {
        wgpuSurfacePresent(renderer->surface);
        wgpuTextureViewRelease(renderer->surface_texture_view);
        wgpuTextureRelease(renderer->surface_texture.texture);
    }
}

void ripple_backend_render_rect(RippleBackendWindowRenderer* window, i32 x, i32 y, i32 w, i32 h, RippleColor color1, RippleColor color2, RippleColor color3, RippleColor color4)
{
    RippleWGPUInstance instance = {
        .pos = { (f32)x, (f32)y },
        .size = { (f32)w, (f32)h },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .image_index = 0
    };
    _ripple_backend_color_to_color(color1, instance.color1);
    _ripple_backend_color_to_color(color2, instance.color2);
    _ripple_backend_color_to_color(color3, instance.color3);
    _ripple_backend_color_to_color(color4, instance.color4);
    vektor_add(window->instances, instance);
}

void ripple_backend_render_image(RippleBackendWindowRenderer* window, i32 x, i32 y, i32 w, i32 h, RippleImage image)
{
    u32 image_index = U32_MAX;
    _RippleImageInstancePair* pair = &window->images.items[window->images.n_items - 1];
    for (u32 i = 0; i < pair->n_images; i++)
    {
        if (pair->images[i] == image)
        {
            image_index = i;
            break;
        }
    }

    if (image_index >= pair->n_images && pair->n_images >= array_len(pair->images))
    {
        vektor_add(window->images, (_RippleImageInstancePair) {
            .images = { [0] = _context.white_pixel.view, [1] = _context.font.view, [2] = image },
            .n_images = 3,
            .instance_index = window->instances.n_items
        });

        image_index = 2;
    }
    else
    {
        if (image_index >= pair->n_images)
        {
            image_index = pair->n_images;
            pair->images[pair->n_images++] = image;
        }

        pair->instance_index = window->instances.n_items;
    }

    vektor_add(window->instances, (RippleWGPUInstance){
        .pos = { (f32)x, (f32)y },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .color1 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color2 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color3 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .color4 = { 1.0f, 1.0f, 1.0f, 1.0f},
        .size = { (f32)w, (f32)h },
        .image_index = image_index
    });
}

void ripple_measure_text(s8 text, f32 font_size, i32* out_w, i32* out_h)
{
    f32 scale = font_size / FONT_SIZE;
    f32 x = 0.0f, y = 0.0f;

    for (u64 i = 0; i < text.size; i++)
    {
        i32 c = text.ptr[i];
        if (c < 32 || c >= 128) continue;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, c - 32, &x, &y, &(stbtt_aligned_quad){ 0 }, 1);
    }

    if (out_w) *out_w = (i32)(x * scale);
    if (out_h) *out_h = (i32)(font_size);
}

void ripple_backend_render_text(RippleBackendWindowRenderer* window, i32 pos_x, i32 pos_y, s8 text, f32 font_size, RippleColor color)
{
    f32 scale = font_size / FONT_SIZE;
    f32 color_arr[4]; _ripple_backend_color_to_color(color, color_arr);
    pos_y += font_size * 0.75f;
    f32 x = 0.0f;
    f32 y = 0.0f;
    for (u64 i = 0; i < text.size; i++)
    {
        i32 c = text.ptr[i];

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, c - 32, &x, &y, &quad, 1);

        vektor_add(window->instances, (RippleWGPUInstance){
            .pos = { pos_x + quad.x0 * scale, pos_y + quad.y0 * scale },
            .size = { (quad.x1 - quad.x0) * scale, (quad.y1 - quad.y0) * scale },
            .uv = { quad.s0, quad.t0, quad.s1, quad.t1 },
            .color1 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color2 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color3 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .color4 = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .image_index = 1
        });
    }
}

#endif // RIPPLE_WGPU_IMPLEMENTATION

#endif // RIPPLE_WGPU_H
