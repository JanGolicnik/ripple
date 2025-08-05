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

#include "../ripple.h"

const f32 FONT_SIZE = 128.0f;
const u32 BITMAP_SIZE = 1024;

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

static void queue_on_submitted_work_done(WGPUQueueWorkDoneStatus status, void* _)
{
    debug("Queued work finished with status: {}", (u32)status);
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
    f32 font_texture_coords[4];
    f32 color[3];
    f32 _padding[1];
} Instance;

const char* shader = "\
struct ShaderData {\
    color: vec4f,\
    resolution: vec2i,\
    time: f32,\
};\
@group(0) @binding(0) var<uniform> shader_data: ShaderData;\
@group(0) @binding(1) var font_texture: texture_2d<f32>;\
@group(0) @binding(2) var font_texture_sampler: sampler;\
\
struct VertexInput {\
    @location(0) position: vec2f,\
};\
\
struct InstanceInput {\
    @location(1) position: vec2f,\
    @location(2) size: vec2f,\
    @location(3) font_texture_coords: vec4f,\
    @location(4) color: vec3f,\
}\
\
struct VertexOutput{\
    @builtin(position) position: vec4f,\
    @location(0) color: vec3f,\
    @location(1) font_texture_coords: vec2f\
};\
\
@vertex \
fn vs_main(v: VertexInput, i: InstanceInput, @builtin(vertex_index) index: u32) -> VertexOutput {\
    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));\
    var position = i.position + v.position * i.size;\
    position = position / resolution;\
    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2f(1.0f, 1.0f);\
    var color = i.color;\
    var font_texture_coords = i.font_texture_coords.xy;\
    if (index == 1) { font_texture_coords = i.font_texture_coords.zy; }\
    else if (index == 2) { font_texture_coords = i.font_texture_coords.zw; }\
    else if (index == 3) { font_texture_coords = i.font_texture_coords.xw; }\
    \
    var out: VertexOutput;\
    out.position = vec4f(position.x, position.y, 0.0, 1.0);\
    out.color = color;\
    out.font_texture_coords = font_texture_coords;\
    return out;\
}\
\
@fragment \
fn fs_main(in: VertexOutput) -> @location(0) vec4f {\
    let color = in.color;\
    let linear_color = pow(color, vec3f(2.2));\
    let alpha = select(\
        1.0,\
        textureSample(font_texture, font_texture_sampler, in.font_texture_coords).x,\
        any(in.font_texture_coords != vec2f(0.0))\
    );\
    return vec4f(linear_color, alpha);\
}\
";

typedef struct {
    f32 color[4];
    i32 resolution[2];
    f32 time;
    f32 _padding[1];
} ShaderData;

typedef struct {
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

    u32 n_instances;
    Instance instances[1024];
} _Window;

static struct {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUBindGroupLayout bind_group_layout;
    WGPUPipelineLayout pipeline_layout;
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;

    struct {
        WGPUTexture texture;
        WGPUTextureView view;
        WGPUSampler sampler;
        stbtt_bakedchar glyphs[96];
    } font;

    MAPA(u64, _Window) windows;
    _Window* current_window;

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

static void on_window_resized(GLFWwindow* glfw_window, i32 w, i32 h)
{
    unused w; unused h;

    u64 window_id = (u64)glfwGetWindowUserPointer(glfw_window);
    _Window* window = mapa_get(_context.windows, &window_id);
    window->should_configure_surface = true;
}

static void _initialize(RippleWindowConfig config)
{
    // GENERAL
    mapa_init(_context.windows, mapa_hash_u64, mapa_cmp_bytes, 0);

    // GLFW
    if (!glfwInit())
        abort("Could not intialize GLFW!");

    // WGPU
    WGPUInstanceDescriptor desc = { 0 };
    _context.instance = wgpuCreateInstance(&desc);
    if (!_context.instance)
        abort("Failed to create WebGPU instance.");

    _context.adapter = get_adapter(_context.instance, (WGPURequestAdapterOptions){ 0 });
    if (!_context.adapter)
        abort("Failed to get the adapter!");
    debug("Successfully got the adapter!");

    _context.device = get_device(_context.adapter);
    if (!_context.device)
        abort("Failed to get the device!");
    debug("Succesfully got the device!");

    _context.queue = wgpuDeviceGetQueue(_context.device);
    wgpuQueueOnSubmittedWorkDone(_context.queue, &queue_on_submitted_work_done, nullptr);

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

        _context.font.texture = wgpuDeviceCreateTexture(_context.device, &(WGPUTextureDescriptor){
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

        _context.font.sampler = wgpuDeviceCreateSampler(_context.device, &(WGPUSamplerDescriptor){
              .addressModeU = WGPUAddressMode_ClampToEdge,
              .addressModeV = WGPUAddressMode_ClampToEdge,
              .addressModeW = WGPUAddressMode_ClampToEdge,
              .magFilter = WGPUFilterMode_Linear,
              .minFilter = WGPUFilterMode_Linear,
              .mipmapFilter = WGPUMipmapFilterMode_Nearest,
              .maxAnisotropy = 1
            });
    }

    _context.bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.device, &(WGPUBindGroupLayoutDescriptor){
            .entryCount = 3,
            .entries = (WGPUBindGroupLayoutEntry[]) {
                [0] = {
                    .binding = 0,
                    .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                    .buffer = {
                        .type = WGPUBufferBindingType_Uniform,
                        .minBindingSize = sizeof(ShaderData)
                    }
                },
                [1] = {
                    .binding = 1,
                    .visibility = WGPUShaderStage_Fragment,
                    .texture = {
                        .sampleType = WGPUTextureSampleType_Float,
                        .viewDimension = WGPUTextureViewDimension_2D,
                    }
                },
                [2] = {
                    .binding = 2,
                    .visibility = WGPUShaderStage_Fragment,
                    .sampler = {
                        .type = WGPUSamplerBindingType_Filtering
                    }
                }

            }
        });

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(_context.device, &(WGPUShaderModuleDescriptor){
            .label = "Shdader descriptor",
            .hintCount = 0,
            .hints = nullptr,
            .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
                .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
                .code = shader
            }
        });

    _context.pipeline_layout = wgpuDeviceCreatePipelineLayout(_context.device, &(WGPUPipelineLayoutDescriptor){
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &_context.bind_group_layout
        });

    _context.pipeline = wgpuDeviceCreateRenderPipeline(_context.device, &(WGPURenderPipelineDescriptor){
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
                        .attributeCount = 4,
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
                                .offset = offsetof(Instance, font_texture_coords)
                            },
                            [3] = {
                                .shaderLocation = 4,
                                .format = WGPUVertexFormat_Float32x3,
                                .offset = offsetof(Instance, color)
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

    _context.vertex_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor) {
           .size = sizeof(vertex_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });
    wgpuQueueWriteBuffer(_context.queue, _context.vertex_buffer, 0, vertex_data, sizeof(vertex_data));

    _context.index_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor) {
           .size = sizeof(index_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
        });
    wgpuQueueWriteBuffer(_context.queue, _context.index_buffer, 0, index_data, sizeof(index_data));

    _context.initialized = true;
}

_Window create_window(u64 id, RippleWindowConfig config)
{
    _Window window = (_Window){ .id = id, .should_configure_surface = true };

    // GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.not_resizable ? GLFW_FALSE : GLFW_TRUE);
    window.window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
    if (!window.window)
        abort("Couldnt no open window!");

    glfwSetWindowUserPointer(window.window, (void*)id);
    glfwSetFramebufferSizeCallback(window.window, &on_window_resized);
    glfwSetMouseButtonCallback(window.window, &mouse_button_callback);

    // WGPU
    window.surface = glfwGetWGPUSurface(_context.instance, window.window);
    window.surface_format = wgpuSurfaceGetPreferredFormat(window.surface, _context.adapter);

    window.instance_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor) {
           .size = sizeof(window.instances),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });

    window.uniform_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor){
            .size = sizeof(window.shader_data),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

    window.bind_group = wgpuDeviceCreateBindGroup(_context.device, &(WGPUBindGroupDescriptor){
            .layout = _context.bind_group_layout,
            .entryCount = 3,
            .entries = (WGPUBindGroupEntry[]){
                [0] = {
                    .binding = 0,
                    .buffer = window.uniform_buffer,
                    .offset = 0,
                    .size = sizeof(window.shader_data)
                },
                [1] = {
                    .binding = 1,
                    .textureView = _context.font.view,
                },
                [2] = {
                    .binding = 2,
                    .sampler = _context.font.sampler
                }
            }
        });

    return window;
}

void ripple_window_begin(u64 id, RippleWindowConfig config)
{
    if (!_context.initialized)
        _initialize(config);

    u64 parent_id = _context.current_window ? _context.current_window->id : 0;

    _context.current_window = mapa_get(_context.windows, &id);
    if (!_context.current_window)
    {
        _context.current_window = mapa_insert(_context.windows, &id, create_window(id, config));
    }

    _context.current_window->parent_id = parent_id;
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
    *width = w;
    *height = h;
}

RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config)
{
    _Window* window = _context.current_window;

    if (window->should_configure_surface)
    {
        glfwGetFramebufferSize(window->window, &window->shader_data.resolution[0], &window->shader_data.resolution[1]);
        configure_surface(_context.device, window->surface, window->surface_format, window->shader_data.resolution[0], window->shader_data.resolution[1]);
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

void *ripple_render_begin()
{
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_context.device, &(WGPUCommandEncoderDescriptor){ .label = "Command encoder"});
    return encoder;
}

void ripple_render_window_begin(u64 id, void* render_context)
{
    unused render_context;

    _context.current_window = mapa_get(_context.windows, &id);
    _context.current_window->n_instances = 0;
}

void ripple_render_window_end(void* render_context)
{
    WGPUCommandEncoder encoder = (WGPUCommandEncoder)render_context;

    wgpuQueueWriteBuffer(_context.queue,
                         _context.current_window->instance_buffer,
                         0,
                         _context.current_window->instances,
                         sizeof(_context.current_window->instances[0]) * _context.current_window->n_instances);

    wgpuSurfaceGetCurrentTexture(_context.current_window->surface, &_context.current_window->surface_texture);

    _context.current_window->surface_texture_view = wgpuTextureCreateView(_context.current_window->surface_texture.texture, &(WGPUTextureViewDescriptor){
            .label = "Surface texture view",
            .format = wgpuTextureGetFormat(_context.current_window->surface_texture.texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        });

    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment){
                .view = _context.current_window->surface_texture_view,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = (WGPUColor){ 1.0f, 0.7f, 0.4f, 1.0f },
                #ifndef WEBGPU_BACKEND_WGPU
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED
                #endif //WEBGPU_BACKEND_WGPU
            },
            .depthStencilAttachment = nullptr
        });

    wgpuRenderPassEncoderSetPipeline(render_pass, _context.pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, _context.vertex_buffer, 0, wgpuBufferGetSize(_context.vertex_buffer));
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, _context.current_window->instance_buffer, 0, wgpuBufferGetSize(_context.current_window->instance_buffer));
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, _context.index_buffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(_context.index_buffer));
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, _context.current_window->bind_group, 0, nullptr);
    wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, _context.current_window->n_instances, 0, 0, 0);

    wgpuRenderPassEncoderEnd(render_pass);
}

void ripple_render_end(void* render_context)
{
    WGPUCommandEncoder encoder = (WGPUCommandEncoder)render_context;

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
    wgpuCommandEncoderRelease(encoder);

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

static void _u32_to_color(u32 color, f32 out_color[3])
{
    out_color[0] = (f32)((color >> 16) & 0xff) / 255.0f;
    out_color[1] = (f32)((color >> 8) & 0xff) / 255.0f;
    out_color[2] = (f32)(color & 0xff) / 255.0f;
}

void ripple_render_rect(i32 x, i32 y, i32 w, i32 h, u32 color)
{
    f32 color_arr[3]; _u32_to_color(color, color_arr);
    _context.current_window->instances[_context.current_window->n_instances++] = (Instance){ .pos = { (f32)x, (f32)y }, .size = { (f32)w, (f32)h }, .color = { color_arr[0], color_arr[1], color_arr[2] } };
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

void ripple_render_text(i32 pos_x, i32 pos_y, const char* text, f32 font_size, u32 color)
{
    f32 scale = font_size / FONT_SIZE;
    f32 color_arr[3]; _u32_to_color(color, color_arr);
    pos_y += font_size * 0.75f;
    f32 x = 0.0f;
    f32 y = 0.0f;
    for (const char* iter = text; *iter; iter++)
    {
        char c = *iter;

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(_context.font.glyphs, BITMAP_SIZE, BITMAP_SIZE, c - 32, &x, &y, &quad, 1);

        _context.current_window->instances[_context.current_window->n_instances++] = (Instance){
            .pos = { pos_x + quad.x0 * scale, pos_y + quad.y0 * scale },
            .size = { (quad.x1 - quad.x0) * scale, (quad.y1 - quad.y0) * scale },
            .font_texture_coords = { quad.s0, quad.t0, quad.s1, quad.t1 },
            .color = { color_arr[0], color_arr[1], color_arr[2] } };
    }
}

#endif // RIPPLE_WGPU_H
