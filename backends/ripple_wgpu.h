#ifndef RIPPLE_WGPU_H
#define RIPPLE_WGPU_H

#include <webgpu/webgpu.h>
#include <glfw/glfw3.h>
#include <glfw3webgpu.h>

#include <math.h>

#include "../ripple.h"

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

static bool window_resized = true;
static void on_window_resized(GLFWwindow* window, i32 w, i32 h)
{
    (void) w; (void) h;
    window_resized = true;
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
\
struct VertexInput {\
    @location(0) position: vec2f,\
};\
\
struct InstanceInput {\
    @location(1) position: vec2f,\
    @location(2) size: vec2f,\
    @location(3) color: vec3f,\
}\
\
struct VertexOutput{\
    @builtin(position) position: vec4f,\
    @location(0) color: vec3f\
};\
\
@vertex \
fn vs_main(v: VertexInput, i: InstanceInput) -> VertexOutput {\
    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));\
    var position = i.position + v.position * i.size;\
    position = position / resolution;\
    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2f(1.0f, 1.0f);\
    var out: VertexOutput;\
    out.position = vec4f(position.x, position.y, 0.0, 1.0);\
    out.color = i.color;\
    return out;\
}\
\
@fragment \
fn fs_main(in: VertexOutput) -> @location(0) vec4f {\
    let color = in.color;\
    let linear_color = pow(color, vec3f(2.2));\
    return vec4f(linear_color, 1.0);\
}\
";

static struct {
    GLFWwindow* window;
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUTextureFormat surface_format;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUBuffer uniform_buffer;
    WGPUBindGroupLayout bind_group_layout;
    WGPUBindGroup bind_group;
    WGPUPipelineLayout pipeline_layout;
    WGPURenderPipeline pipeline;
    WGPUBuffer vertex_buffer;
    WGPUBuffer index_buffer;
    WGPUBuffer instance_buffer;

    Instance instances[1024];
    u32 n_instances;

    struct {
        f32 color[4];
        i32 resolution[2];
        f32 time;
        f32 _padding[1];
    } shader_data;

    struct {
        bool left_pressed;
        bool right_pressed;
        bool middle_pressed;
        bool left_released;
        bool right_released;
        bool middle_released;
    };

    bool initialized;
    bool render_begin;
} _context;

static void mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
    if (action == GLFW_PRESS)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            _context.left_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            _context.right_pressed = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            _context.middle_pressed = true;
        return;
    }

    if (action == GLFW_RELEASE)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            _context.left_released = true;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            _context.right_released = true;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            _context.middle_released = true;
        return;
    }
}

static void _initialize(RippleWindowConfig config)
{
    // GLFW
    if (!glfwInit())
        abort("Could not intialize GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.not_resizable ? GLFW_FALSE : GLFW_TRUE);
    _context.window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
    if (!_context.window)
        abort("Couldnt no open window!");

    glfwSetFramebufferSizeCallback(_context.window, &on_window_resized);
    glfwSetMouseButtonCallback(_context.window, &mouse_button_callback);

    // WGPU
    WGPUInstanceDescriptor desc = { 0 };
    _context.instance = wgpuCreateInstance(&desc);
    if (!_context.instance)
        abort("Failed to create WebGPU instance.");

    _context.surface = glfwGetWGPUSurface(_context.instance, _context.window);
    WGPUAdapter adapter = get_adapter(_context.instance, (WGPURequestAdapterOptions){ .compatibleSurface = _context.surface });
    if (!adapter)
        abort("Failed to get the adapter!");
    debug("Successfully got the adapter!");

    _context.surface_format = wgpuSurfaceGetPreferredFormat(_context.surface, adapter);

    _context.device = get_device(adapter);
    if (!_context.device)
        abort("Failed to get the device!");
    debug("Succesfully got the device!");

    wgpuAdapterRelease(adapter);

    _context.queue = wgpuDeviceGetQueue(_context.device);
    wgpuQueueOnSubmittedWorkDone(_context.queue, &queue_on_submitted_work_done, nullptr);

    // SHADERS
    _context.uniform_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor){
            .size = sizeof(_context.shader_data),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

    _context.bind_group_layout = wgpuDeviceCreateBindGroupLayout(_context.device, &(WGPUBindGroupLayoutDescriptor){
            .entryCount = 1,
            .entries = &(WGPUBindGroupLayoutEntry) {
                .binding = 0,
                .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                .buffer = {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof(_context.shader_data)
                }
            }
        });
    _context.bind_group = wgpuDeviceCreateBindGroup(_context.device, &(WGPUBindGroupDescriptor){
            .layout = _context.bind_group_layout,
            .entryCount = 1,
            .entries = &(WGPUBindGroupEntry){
                .binding = 0,
                .buffer = _context.uniform_buffer,
                .offset = 0,
                .size = sizeof(_context.shader_data)
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
                        .attributeCount = 3,
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
                    .format = _context.surface_format,
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

    _context.instance_buffer = wgpuDeviceCreateBuffer(_context.device, &(WGPUBufferDescriptor) {
           .size = sizeof(_context.instances),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });

    _context.initialized = true;
}

void ripple_window_begin(RippleWindowConfig config)
{
    if (!_context.initialized)
        _initialize(config);

    // handle multiple windows

    // set window size
}

void ripple_get_window_size(u32* width, u32* height)
{
    i32 w, h; glfwGetFramebufferSize(_context.window, &w, &h);
    *width = w;
    *height = h;
}

RippleWindowState ripple_update_window_state(RippleWindowState state, RippleWindowConfig config)
{
    if (window_resized)
    {
        glfwGetFramebufferSize(_context.window, &_context.shader_data.resolution[0], &_context.shader_data.resolution[1]);
        configure_surface(_context.device, _context.surface, _context.surface_format, _context.shader_data.resolution[0], _context.shader_data.resolution[1]);
        window_resized = false;
    }

    state.should_close = glfwWindowShouldClose(_context.window);

    glfwPollEvents();
//   f32 prev_time = shader_data.time;
    _context.shader_data.time = (f32)glfwGetTime();
//    f32 dt = shader_data.time - prev_time;
    _context.shader_data.color[0] = sinf(_context.shader_data.time) * 0.5f + 0.5f;
    _context.shader_data.color[1] = sinf(_context.shader_data.time * 2.5f) * 0.25f + 0.5f;
    _context.shader_data.color[2] = sinf(_context.shader_data.time * 5.0f) * 0.1f + 0.5f;
    wgpuQueueWriteBuffer(_context.queue, _context.uniform_buffer, 0, &_context.shader_data, sizeof(_context.shader_data));

    return state;
}

RippleCursorState ripple_update_cursor_state(RippleCursorState state)
{
    double x, y; glfwGetCursorPos(_context.window, &x, &y);
    state.x = (i32)x;
    state.y = (i32)y;
    state.left.pressed = _context.left_pressed;
    state.right.pressed = _context.right_pressed;
    state.middle.pressed = _context.middle_pressed;
    state.left.released = _context.left_released;
    state.right.released = _context.right_released;
    state.middle.released = _context.middle_released;

    _context.left_pressed = false;
    _context.right_pressed = false;
    _context.middle_pressed = false;
    _context.left_released = false;
    _context.right_released = false;
    _context.middle_released = false;

    return state;
}

void *ripple_render_begin()
{
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_context.device, &(WGPUCommandEncoderDescriptor){ .label = "Command encoder"});
    return encoder;
}

void ripple_render_window_begin(void* render_context)
{
    _context.n_instances = 0;

    WGPUCommandEncoder encoder = (WGPUCommandEncoder)render_context;
    (void)encoder;
}

void ripple_render_window_end(void* render_context)
{
    WGPUCommandEncoder encoder = (WGPUCommandEncoder)render_context;

    wgpuQueueWriteBuffer(_context.queue, _context.instance_buffer, 0, _context.instances, sizeof(_context.instances[0]) * _context.n_instances);

    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(_context.surface, &surface_texture);

    WGPUTextureView surface_texture_view = wgpuTextureCreateView(surface_texture.texture, &(WGPUTextureViewDescriptor){
            .label = "Surface texture view",
            .format = wgpuTextureGetFormat(surface_texture.texture),
            .dimension = WGPUTextureViewDimension_2D,
            .mipLevelCount = 1,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        });

    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &(WGPURenderPassDescriptor){
            .colorAttachmentCount = 1,
            .colorAttachments = &(WGPURenderPassColorAttachment){
                .view = surface_texture_view,
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
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, _context.instance_buffer, 0, wgpuBufferGetSize(_context.instance_buffer));
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, _context.index_buffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(_context.index_buffer));
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, _context.bind_group, 0, nullptr);
    wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, _context.n_instances, 0, 0, 0);

    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(_context.queue, 1, &command);
    wgpuCommandBufferRelease(command);

    wgpuSurfacePresent(_context.surface);
    wgpuTextureViewRelease(surface_texture_view);
    wgpuTextureRelease(surface_texture.texture);
}

void ripple_render_end(void* render_context)
{
    /*
    WGPUCommandEncoder encoder = (WGPUCommandEncoder)render_context;

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(_context.queue, 1, &command);
    wgpuCommandBufferRelease(command);
     */
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
    _context.instances[_context.n_instances++] = (Instance){ .pos = { (f32)x, (f32)y }, .size = { (f32)w, (f32)h }, .color = { color_arr[0], color_arr[1], color_arr[2] } };
}

void ripple_measure_text(const char* text, f32 font_size, i32* out_w, i32* out_h)
{
    *out_w = (str_len(text) * font_size) / 2;
    *out_h = font_size;
}

void ripple_render_text(i32 x, i32 y, const char* text, f32 font_size, u32 color)
{
    // add to instance q
}

#endif // RIPPLE_WGPU_H
