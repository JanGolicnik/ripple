#ifndef RIPPLE_WGPU_H
#define RIPPLE_WGPU_H

#include <webgpu/webgpu.h>
#include <glfw/glfw3.h>
#include <glfw3webgpu.h>

#ifndef STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include <stb/stb_truetype.h>

#include <math.h>

#ifdef RIPPLE_IMPLEMENTATION
#define RIPPLE_WGPU_IMPLEMENTATION
#endif // RIPPLE_IMPLEMENTATION

typedef struct {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
} RippleBackendConfig;

typedef struct {
    WGPUCommandEncoder encoder;
} RippleRenderData;

typedef u64 RippleImage;

#include "../ripple.h"

const f32 FONT_SIZE = 128.0f;
const u32 BITMAP_SIZE = 1024;

RippleImage ripple_register_image(WGPUTextureView view);

#ifdef RIPPLE_WGPU_IMPLEMENTATION

typedef struct {
    WGPUAdapter adapter;
    bool request_done;
} RequestAdapterUserData;

static void request_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata)
{
    if (status != WGPURequestAdapterStatus_Success)
        abort("Failed to get WebGPU adapter");

    RequestAdapterUserData* callback_user_data = userdata;
    callback_user_data->adapter = adapter;
    callback_user_data->request_done = true;
}

static WGPUAdapter get_adapter(WGPUInstance instance, WGPURequestAdapterOptions request_adapter_options)
{
    RequestAdapterUserData request_adapter_user_data = { 0 };
    wgpuInstanceRequestAdapter(instance, &request_adapter_options, request_adapter_callback, &request_adapter_user_data);
    return request_adapter_user_data.adapter;
}

typedef struct {
    WGPUDevice device;
    bool request_done;
} RequestDeviceUserData;

static void request_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata)
{
    if (status != WGPURequestDeviceStatus_Success)
    {
        abort("Failed to get WebGPU device");
    }

    RequestDeviceUserData* callback_user_data = userdata;
    callback_user_data->device = device;
    callback_user_data->request_done = true;
}

static void device_uncaptured_error_callback(WGPUErrorType type, char const* message, void* _)
{
    error("Uncaptured device error ({}): {}", (u32)type, message ? message : "");
}

static WGPUDevice get_device(WGPUAdapter adapter)
{
    WGPUDeviceDescriptor device_descriptor = { .label = "Device :D", .defaultQueue.label = "Default queue" };
    RequestDeviceUserData request_device_user_data = { 0 };
    wgpuAdapterRequestDevice(adapter, &device_descriptor, &request_device_callback, &request_device_user_data);

    WGPUDevice device = request_device_user_data.device;
    wgpuDeviceSetUncapturedErrorCallback(device, &device_uncaptured_error_callback, nullptr);
    return device;
}

RippleBackendConfig ripple_get_default_backend_config()
{
    RippleBackendConfig config = { 0 };

    config.instance = wgpuCreateInstance(&(WGPUInstanceDescriptor) { 0 });
    if (!config.instance)
        abort("Failed to create WebGPU instance.");
    debug("Successfully created the WebGPU instance!");

    config.adapter = get_adapter(config.instance, (WGPURequestAdapterOptions){ 0 });
    if (!config.adapter)
        abort("Failed to get the adapter!");
    debug("Successfully got the adapter!");

    config.device = get_device(config.adapter);
    if (!config.device)
        abort("Failed to get the device!");
    debug("Succesfully got the device!");

    return config;
}

static void configure_surface(WGPUDevice device, WGPUSurface surface, WGPUTextureFormat format, u32 width, u32 height)
{
    wgpuSurfaceConfigure(surface, &(WGPUSurfaceConfiguration){
            .width = width,
            .height = height,
            .format = format,
            .usage = WGPUTextureUsage_RenderAttachment,
            .device = device,
            .presentMode = WGPUPresentMode_Fifo,
            .alphaMode = WGPUCompositeAlphaMode_Auto
        });
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
    f32 color[4];
    f32 color_padding[1];
    u32 image_index;
    u32 image_index_padding[3];
} Instance;

const char* shader = "\
struct ShaderData {\
    color: vec4f,\
    resolution: vec2i,\
    time: f32,\
};\
@group(0) @binding(0) var<uniform> shader_data: ShaderData;\
\
@group(1) @binding(0) var texture1: texture_2d<f32>;\
@group(1) @binding(1) var texture2: texture_2d<f32>;\
@group(1) @binding(2) var texture3: texture_2d<f32>;\
@group(1) @binding(3) var texture4: texture_2d<f32>;\
@group(1) @binding(4) var texture5: texture_2d<f32>;\
\
@group(2) @binding(0) var texture_sampler: sampler;\
\
struct VertexInput {\
    @location(0) position: vec2f,\
};\
\
struct InstanceInput {\
    @location(1) position: vec2f,\
    @location(2) size: vec2f,\
    @location(3) uv: vec4f,\
    @location(4) color: vec4f,\
    @location(5) image_index: u32,\
}\
\
struct VertexOutput{\
    @builtin(position) position: vec4f,\
    @location(0) color: vec4f,\
    @location(2) uv: vec2f,\
    @location(3) image_index: u32\
};\
\
@vertex \
fn vs_main(v: VertexInput, i: InstanceInput, @builtin(vertex_index) index: u32) -> VertexOutput {\
    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));\
    var position = i.position + v.position * i.size;\
    position = position / resolution;\
    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2f(1.0f, 1.0f);\
    var color = i.color;\
    var uv = i.uv.xy;\
    if (index == 1) { uv = i.uv.zy; }\
    else if (index == 2) { uv = i.uv.zw; }\
    else if (index == 3) { uv = i.uv.xw; }\
    \
    var out: VertexOutput;\
    out.position = vec4f(position.x, position.y, 0.0, 1.0);\
    out.color = color;\
    out.uv = uv;\
    out.image_index = i.image_index;\
    return out;\
}\
\
@fragment \
fn fs_main(in: VertexOutput) -> @location(0) vec4f {\
    var tex_color: vec4f;\
    switch in.image_index {\
        case 0u { tex_color = textureSample(texture1, texture_sampler, in.uv); }\
        case 1u { tex_color = vec4f(textureSample(texture2, texture_sampler, in.uv).r); }\
        case 2u { tex_color = textureSample(texture3, texture_sampler, in.uv); }\
        case 3u { tex_color = textureSample(texture4, texture_sampler, in.uv); }\
        case 4u { tex_color = textureSample(texture5, texture_sampler, in.uv); }\
        default { tex_color = vec4f(1.0, 1.0, 1.0, 1.0); }\
    }\
    let color = tex_color * in.color;\
    let linear_color = pow(color.rgb, vec3f(2.2));\
    return vec4f(linear_color, color.a);\
}\
";

    //let color = select(in.color, textureSample(texture, texture_sampler, in.uv).rgb, in.image_index != 0);

typedef struct {
    f32 color[4];
    i32 resolution[2];
    f32 time;
    f32 _padding[1];
} ShaderData;

typedef struct {
    RippleImage images[5];
    u32 n_images;
    u32 instance_index;
} _RippleImageInstancePair;

typedef struct {
    RippleWindowConfig config;

    struct {
        i32 x;
        i32 y;
    } prev_config;

    u64 id;
    u64 parent_id;

    GLFWwindow* window;
    WGPUSurface surface;
    WGPUTextureFormat surface_format;

    // these two are only set inbetween render_window_end and render_end
    WGPUSurfaceTexture surface_texture;
    WGPUTextureView surface_texture_view;


    WGPUBuffer instance_buffer;
    WGPUBuffer uniform_buffer;
    WGPUBindGroup bind_group;

    struct {
        bool left_pressed;
        bool right_pressed;
        bool middle_pressed;
        bool left_released;
        bool right_released;
        bool middle_released;
    };

    ShaderData shader_data;

    bool render_begin;
    bool should_configure_surface;

    u32 instance_buffer_size;
    VEKTOR(Instance) instances;

    VEKTOR( _RippleImageInstancePair ) images;
} _Window;

static struct {
    RippleBackendConfig config;
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
        RippleImage image;
    } white_pixel;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        RippleImage image;
        stbtt_bakedchar glyphs[96];
    } font;

    MAPA(u64, _Window) windows;
    _Window* current_window;

    MAPA ( RippleImage, WGPUTextureView ) image_textures;

    bool initialized;
} _context;

static void mouse_button_callback(GLFWwindow* glfw_window, i32 button, i32 action, i32 mods)
{
    unused mods;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    _Window* window = mapa_get(_context.windows, &window_id);

    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window->left_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window->right_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window->middle_pressed = true;
        return;
    }

    if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            window->left_released = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            window->right_released = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            window->middle_released = true;
        return;
    }
}

static void window_pos_callback(GLFWwindow* glfw_window, i32 x, i32 y)
{
    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    _Window* window = mapa_get(_context.windows, &window_id);

    if (window->config.x) *window->config.x = x;
    if (window->config.y) *window->config.y = y;
}

static void on_window_resized(GLFWwindow* glfw_window, i32 w, i32 h)
{
    unused w; unused h;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    _Window* window = mapa_get(_context.windows, &window_id);
    window->should_configure_surface = true;
}

void ripple_backend_initialize(RippleBackendConfig config)
{
    debug("INITIALIZNG BACKEND");
    // GENERAL
    mapa_init(_context.windows, mapa_hash_u64, mapa_cmp_bytes, 0);
    mapa_init(_context.image_textures, mapa_hash_u64, mapa_cmp_bytes, 0);

    // GLFW
    if (!glfwInit())
        abort("Could not intialize GLFW!");

    // WGPU
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
                .label = "fontAtlas",
                .size = (WGPUExtent3D){ .width = BITMAP_SIZE, .height = BITMAP_SIZE, .depthOrArrayLayers = 1 },
                .format = WGPUTextureFormat_R8Unorm,
                .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
                .dimension = WGPUTextureDimension_2D,
                .mipLevelCount = 1,
                .sampleCount = 1
            });

        wgpuQueueWriteTexture(_context.queue, &(WGPUImageCopyTexture){
                .texture = _context.font.texture,
                .aspect = WGPUTextureAspect_All
            },
            bitmap_buffer,
            BITMAP_SIZE * BITMAP_SIZE,
            &(WGPUTextureDataLayout){
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

        _context.font.image = ripple_register_image(_context.font.view);
    }

    {
        _context.white_pixel.texture = wgpuDeviceCreateTexture(_context.config.device, &(WGPUTextureDescriptor){
                .label = "white_pixel",
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
        wgpuQueueWriteTexture(_context.queue, &(WGPUImageCopyTexture){
                .texture = _context.white_pixel.texture,
                .aspect = WGPUTextureAspect_All
            },
            &white,
            4,
            &(WGPUTextureDataLayout){
                .bytesPerRow = 4,
                .rowsPerImage = 1
            },
            &(WGPUExtent3D){
                .width = 1,
                .height = 1,
                .depthOrArrayLayers = 1
            }
        );

        _context.white_pixel.image = ripple_register_image(_context.white_pixel.view);
    }

    _context.bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.config.device, &(WGPUBindGroupLayoutDescriptor){
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

    _context.image_bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.config.device, &(WGPUBindGroupLayoutDescriptor)
        {
            .entryCount = 5,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [1] = {
                    .binding = 1,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [2] = {
                    .binding = 2,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [3] = {
                    .binding = 3,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    },
                },
                [4] = {
                    .binding = 4,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
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
                    .type = WGPUSamplerBindingType_Filtering
                }
            }
        });

    _context.sampler = wgpuDeviceCreateSampler(_context.config.device, &(WGPUSamplerDescriptor){
            .addressModeU = WGPUAddressMode_ClampToEdge,
            .addressModeV = WGPUAddressMode_ClampToEdge,
            .addressModeW = WGPUAddressMode_ClampToEdge,
            .magFilter = WGPUFilterMode_Linear,
            .minFilter = WGPUFilterMode_Linear,
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
            .label = "Shdader descriptor",
            .hintCount = 0,
            .hints = nullptr,
            .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
                .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
                .code = shader
            }
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
                .entryPoint = "vs_main",
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
                        .attributeCount = 5,
                        .attributes = (WGPUVertexAttribute[]) {
                            [0] = {
                                .shaderLocation = 1,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = 0
                            },
                            [1] = {
                                .shaderLocation = 2,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = offsetof(Instance, size)
                            },
                            [2] = {
                                .shaderLocation = 3,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(Instance, uv)
                            },
                            [3] = {
                                .shaderLocation = 4,
                                .format = WGPUVertexFormat_Float32x4,
                                .offset = offsetof(Instance, color)
                            },
                            [4] = {
                                .shaderLocation = 5,
                                .format = WGPUVertexFormat_Uint32,
                                .offset = offsetof(Instance, image_index)
                            }
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

    _context.initialized = true;
}

_Window create_window(u64 id, RippleWindowConfig config)
{
    _Window window = (_Window){ .id = id, .should_configure_surface = true, .config = config };

    // GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.not_resizable ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, config.hide_title ? GLFW_FALSE : GLFW_TRUE );
    if (config.set_position)
    {
        glfwWindowHint(GLFW_POSITION_X, *config.x);
        glfwWindowHint(GLFW_POSITION_Y, *config.y);
    }
    window.window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
    if (!window.window)
        abort("Couldnt no open window!");

    glfwSetWindowUserPointer(window.window, (void*)id);
    glfwSetFramebufferSizeCallback(window.window, &on_window_resized);
    glfwSetMouseButtonCallback(window.window, &mouse_button_callback);
    glfwSetWindowPosCallback(window.window, &window_pos_callback);

    // WGPU
    window.surface = glfwGetWGPUSurface(_context.config.instance, window.window);
    window.surface_format = wgpuSurfaceGetPreferredFormat(window.surface, _context.config.adapter);

    window.uniform_buffer = wgpuDeviceCreateBuffer(_context.config.device, &(WGPUBufferDescriptor){
            .size = sizeof(window.shader_data),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

    window.bind_group = wgpuDeviceCreateBindGroup(_context.config.device, &(WGPUBindGroupDescriptor){
            .layout = _context.bind_group_layout,
            .entryCount = 1,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .buffer = window.uniform_buffer,
                    .offset = 0,
                    .size = sizeof(window.shader_data)
                }
            }
        });

    if (config.x) window.prev_config.x = *config.x;
    if (config.y) window.prev_config.y = *config.y;

    return window;
}

void ripple_window_begin(u64 id, RippleWindowConfig config)
{
    if (!_context.initialized)
        ripple_backend_initialize(ripple_get_default_backend_config());

    u64 parent_id = _context.current_window ? _context.current_window->id : 0;

    _context.current_window = mapa_get(_context.windows, &id);
    if (!_context.current_window)
    {
        _context.current_window = mapa_insert(_context.windows, &id, create_window(id, config));
    }

    _context.current_window->parent_id = parent_id;

    if ((config.x && _context.current_window->prev_config.x != *config.x) ||
        (config.y && _context.current_window->prev_config.y != *config.y))
    {
        glfwSetWindowPos(_context.current_window->window, *config.x, *config.y);
    }

    bool width_changed = _context.current_window->config.width != config.width;
    bool height_changed = _context.current_window->config.height != config.height;
    if (width_changed && height_changed)
    {
        glfwSetWindowSize(_context.current_window->window, config.width, config.height);
    }
    else if (width_changed)
    {
        u32 h; ripple_get_window_size(nullptr, &h);
        glfwSetWindowSize(_context.current_window->window, config.width, h);
    }
    else if (height_changed)
    {
        u32 w; ripple_get_window_size(&w, nullptr);
        glfwSetWindowSize(_context.current_window->window, w, config.height);
    }

    _context.current_window->config = config;
}

void ripple_window_end()
{
    _context.current_window = _context.current_window->parent_id ?
        mapa_get(_context.windows, &_context.current_window->parent_id) :
        nullptr;
}

void ripple_window_close(u64 id)
{
    _Window* window = mapa_get(_context.windows, &id);
    glfwDestroyWindow(window->window);
    mapa_remove(_context.windows, &id);
}

void ripple_get_window_size(u32* width, u32* height)
{
    i32 w, h;
    glfwGetFramebufferSize(_context.current_window->window, &w, &h);
    if (width) *width = w;
    if (height) *height = h;
}

RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config)
{
    _Window* window = _context.current_window;

    if (window->should_configure_surface)
    {
        glfwGetFramebufferSize(window->window, &window->shader_data.resolution[0], &window->shader_data.resolution[1]);
        configure_surface(_context.config.device, window->surface, window->surface_format, window->shader_data.resolution[0], window->shader_data.resolution[1]);
        window->should_configure_surface = false;
    }

    state.is_open = !glfwWindowShouldClose(_context.current_window->window);

    glfwPollEvents();
    window->shader_data.time = (f32)glfwGetTime();
    window->shader_data.color[0] = sinf(window->shader_data.time) * 0.5f + 0.5f;
    window->shader_data.color[1] = sinf(window->shader_data.time * 2.5f) * 0.25f + 0.5f;
    window->shader_data.color[2] = sinf(window->shader_data.time * 5.0f) * 0.1f + 0.5f;
    wgpuQueueWriteBuffer(_context.queue, window->uniform_buffer, 0, &window->shader_data, sizeof(window->shader_data));

    return state;
}

RippleCursorState ripple_update_cursor_state(RippleCursorState state)
{
    double x, y; glfwGetCursorPos(_context.current_window->window, &x, &y);
    state.x = (i32)x;
    state.y = (i32)y;
    state.left.pressed = _context.current_window->left_pressed;
    state.right.pressed = _context.current_window->right_pressed;
    state.middle.pressed = _context.current_window->middle_pressed;
    state.left.released = _context.current_window->left_released;
    state.right.released = _context.current_window->right_released;
    state.middle.released = _context.current_window->middle_released;

    _context.current_window->left_pressed = false;
    _context.current_window->right_pressed = false;
    _context.current_window->middle_pressed = false;
    _context.current_window->left_released = false;
    _context.current_window->right_released = false;
    _context.current_window->middle_released = false;

    return state;
}

RippleRenderData ripple_render_begin()
{
    return (RippleRenderData){
        .encoder = wgpuDeviceCreateCommandEncoder(_context.config.device, &(WGPUCommandEncoderDescriptor){ .label = "Command encoder"})
    };
}

void ripple_render_window_begin(u64 id, RippleRenderData render_data)
{
    unused render_data;

    _context.current_window = mapa_get(_context.windows, &id);
    vektor_clear(_context.current_window->instances);
    vektor_clear(_context.current_window->images);
    vektor_add(_context.current_window->images, (_RippleImageInstancePair) {
        .images = { [0] = _context.white_pixel.image, [1] = _context.font.image },
        .n_images = 2,
        .instance_index = 0
    });
}

void ripple_render_window_end(RippleRenderData render_data)
{
    _Window* window = _context.current_window;

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
            .label = "Surface texture view",
            .format = wgpuTextureGetFormat(window->surface_texture.texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        });

    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(render_data.encoder, &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment){
                .view = window->surface_texture_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = (WGPUColor){ 1.0f, 0.7f, 0.4f, 1.0f },
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
                .textureView = *mapa_get(_context.image_textures, &image_pair->images[min(j, image_pair->n_images - 1)])
            };
        }

        bind_groups[i] = wgpuDeviceCreateBindGroup(_context.config.device, &(WGPUBindGroupDescriptor)
            {
                .layout = _context.image_bind_group_layout,
                .entryCount = array_len(image_textures),
                .entries = image_textures
            });

        u32 n_instances = ((i == window->images.n_items - 1) ? window->instances.n_items : image_pair->instance_index) - instance_index;

        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, window->instance_buffer, 0, n_instances * sizeof(Instance));
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

void ripple_render_end(RippleRenderData render_data)
{
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(render_data.encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
    wgpuCommandEncoderRelease(render_data.encoder);

    wgpuQueueSubmit(_context.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    for (u32 window_i = 0; window_i < _context.windows.size; window_i++)
    {
        _Window* window = mapa_get_at_index(_context.windows, window_i);
        if (!window) continue;
        wgpuSurfacePresent(window->surface);
        wgpuTextureViewRelease(window->surface_texture_view);
        wgpuTextureRelease(window->surface_texture.texture);
    }
}

static void _ripple_color_to_color(RippleColor color, f32 out_color[4])
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

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, RippleColor color)
{
    f32 color_arr[4]; _ripple_color_to_color(color, color_arr);
    vektor_add(_context.current_window->instances, (Instance){
        .pos = { (f32)x, (f32)y },
        .size = { (f32)w, (f32)h },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .color = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
        .image_index = 0
    });
}

void ripple_render_image(i32 x, i32 y, i32 w, i32 h, RippleImage image)
{
    _Window *window = _context.current_window;

    u32 image_index = U32_MAX;
    _RippleImageInstancePair* pair = &window->images.items[_context.current_window->images.n_items - 1];
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
            .images = { [0] = _context.white_pixel.image, [1] = _context.font.image, [2] = image },
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

        pair->instance_index = _context.current_window->instances.n_items;
    }

    vektor_add(window->instances, (Instance){
        .pos = { (f32)x, (f32)y },
        .uv = { 0.0f, 0.0f, 1.0f, 1.0f },
        .color = { 1.0f, 1.0f, 1.0f, 1.0f},
        .size = { (f32)w, (f32)h },
        .image_index = image_index
    });
}

void ripple_measure_text(const char* text, f32 font_size, i32* out_w, i32* out_h)
{
    f32 scale = font_size / FONT_SIZE;
    f32 x = 0.0f, y = 0.0f;

    for (const char* iter = text; *iter; iter++) {
        i32 c = *iter;
        if (c < 32 || c >= 128) continue;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, c - 32, &x, &y, &(stbtt_aligned_quad){ 0 }, 1);
    }

    *out_w = (i32)(x * scale);
    *out_h = (i32)(font_size);
}

void ripple_render_text(i32 pos_x, i32 pos_y, const char* text, f32 font_size, RippleColor color)
{
    f32 scale = font_size / FONT_SIZE;
    f32 color_arr[4]; _ripple_color_to_color(color, color_arr);
    pos_y += font_size * 0.75f;
    f32 x = 0.0f;
    f32 y = 0.0f;
    for (const char* iter = text; *iter; iter++)
    {
        char c = *iter;

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, c - 32, &x, &y, &quad, 1);

        vektor_add(_context.current_window->instances, (Instance){
            .pos = { pos_x + quad.x0 * scale, pos_y + quad.y0 * scale },
            .size = { (quad.x1 - quad.x0) * scale, (quad.y1 - quad.y0) * scale },
            .uv = { quad.s0, quad.t0, quad.s1, quad.t1 },
            .color = { color_arr[0], color_arr[1], color_arr[2], color_arr[3] },
            .image_index = 1
        });
    }
}


RippleImage ripple_register_image(WGPUTextureView view)
{
    u64 hash = hash_u64((u64)view);

    if (mapa_get(_context.image_textures, &hash))
    {
        abort("image already registered");
    }

    (void)mapa_insert(_context.image_textures, &hash, view);

    return hash;
}

#endif // RIPPLE_WGPU_IMPLEMENTATION

#endif // RIPPLE_WGPU_H
