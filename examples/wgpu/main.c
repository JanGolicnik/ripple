#include <webgpu/webgpu.h>
#include <glfw/glfw3.h>
#include <glfw3webgpu.h>

#include <math.h>

#include <stdio.h>
#include <marrow/marrow.h>

typedef struct {
    WGPUAdapter adapter;
    bool request_done;
} RequestAdapterUserData;

void request_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata)
{
    if (status != WGPURequestAdapterStatus_Success)
        abort("Failed to get WebGPU adapter");

    RequestAdapterUserData* callback_user_data = userdata;
    callback_user_data->adapter = adapter;
    callback_user_data->request_done = true;
}

WGPUAdapter get_adapter(WGPUInstance instance, WGPURequestAdapterOptions request_adapter_options)
{
    RequestAdapterUserData request_adapter_user_data = { 0 };
    wgpuInstanceRequestAdapter(instance, &request_adapter_options, request_adapter_callback, &request_adapter_user_data);
    return request_adapter_user_data.adapter;
}

typedef struct {
    WGPUDevice device;
    bool request_done;
} RequestDeviceUserData;

void request_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata)
{
    if (status != WGPURequestDeviceStatus_Success)
    {
        abort("Failed to get WebGPU device");
    }

    RequestDeviceUserData* callback_user_data = userdata;
    callback_user_data->device = device;
    callback_user_data->request_done = true;
}

void device_uncaptured_error_callback(WGPUErrorType type, char const* message, void* _)
{
    error("Uncaptured device error ({}): {}", (u32)type, message ? message : "");
}

WGPUDevice get_device(WGPUAdapter adapter)
{
    WGPUDeviceDescriptor device_descriptor = { .label = "Device :D", .defaultQueue.label = "Default queue" };
    RequestDeviceUserData request_device_user_data = { 0 };
    wgpuAdapterRequestDevice(adapter, &device_descriptor, &request_device_callback, &request_device_user_data);

    WGPUDevice device = request_device_user_data.device;
    wgpuDeviceSetUncapturedErrorCallback(device, &device_uncaptured_error_callback, nullptr);
    return device;
}

void queue_on_submitted_work_done(WGPUQueueWorkDoneStatus status, void* _)
{
    debug("Queued work finished with status: {}", (u32)status);
}

bool window_resized = true;
void on_window_resized(GLFWwindow* window, i32 w, i32 h)
{
    (void) w; (void) h;
    window_resized = true;
}

void configure_surface(WGPUDevice device, WGPUSurface surface, WGPUTextureFormat format, u32 width, u32 height)
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
}\
\
struct VertexOutput{\
    @builtin(position) position: vec4f,\
    @location(0) color: vec3f\
};\
\
@vertex \
fn vs_main(v: VertexInput, i: InstanceInput) -> VertexOutput {\
    let position = v.position + vec2f(sin(shader_data.time) * 0.4f, 0.0) + i.position;\
    let aspect = f32(shader_data.resolution.x) / f32(shader_data.resolution.y);\
    let color = min(vec3f(1.0f, 0.3f, 0.003f) * length(position), vec3f(1.0f));\
    var out: VertexOutput;\
    out.position = vec4f(position.x / aspect, position.y, 0.0, 1.0);\
    out.color = color;\
    return out;\
}\
\
@fragment \
fn fs_main(in: VertexOutput) -> @location(0) vec4f {\
    let color = in.color * shader_data.color.rgb;\
    let linear_color = pow(color, vec3f(2.2));\
    return vec4f(linear_color, 1.0);\
}\
";

f32 vertex_data[] = {
    -0.5, -0.5,
    +0.5, -0.5,
    +0.5, +0.5,
    -0.5, +0.5,
};
const u32 vertex_data_size = 2 * sizeof(vertex_data[0]);
const u32 vertex_count = sizeof(vertex_data) / vertex_data_size;

uint16_t index_data[] = {
    0, 1, 2,
    0, 2, 3
};
const u32 index_data_size = 1 * sizeof(index_data[0]);
const u32 index_count = sizeof(index_data) / index_data_size;

typedef struct {
    f32 x;
    f32 y;
} Instance;
Instance instance_data[] = { {0.0f, 0.0f}, { 0.5f, 0.0f }, { 0.0f, 0.4f }};
const u32 instance_data_size = sizeof(instance_data[0]);
const u32 instance_count = sizeof(instance_data) / instance_data_size;

typedef struct {
    f32 color[4];
    i32 resolution[2];
    f32 time;
    f32 _padding[1];
} ShaderData;

int main(int argc, char* argv[])
{
    // SET UP GLFW
    if (!glfwInit())
        abort("Could not intialize GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 800, "HIIII HIII HIII!!!H IIIII HIH IHI HII!!!I HIIIII", nullptr, nullptr);
    if (!window)
        abort("Couldnt no open window!");

    glfwSetFramebufferSizeCallback(window, &on_window_resized);

    // SET UP WGPU
    WGPUInstanceDescriptor desc = { 0 };
    WGPUInstance instance = wgpuCreateInstance(&desc);
    if (!instance)
        abort("Failed to create WebGPU instance.");

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);
    WGPUAdapter adapter = get_adapter(instance, (WGPURequestAdapterOptions){ .compatibleSurface = surface });
    if (!adapter)
        abort("Failed to get the adapter!");
    debug("Successfully got the adapter!");

    WGPUTextureFormat surface_format = wgpuSurfaceGetPreferredFormat(surface, adapter);

    WGPUDevice device = get_device(adapter);
    if (!device)
        abort("Failed to get the device!");
    debug("Succesfully got the device!");

    wgpuAdapterRelease(adapter);

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    wgpuQueueOnSubmittedWorkDone(queue, &queue_on_submitted_work_done, nullptr);

    // SET UP PIPELINES
    WGPUBuffer uniform_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor){
            .size = sizeof(ShaderData),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        });

    WGPUBindGroupLayout bind_group_layout = wgpuDeviceCreateBindGroupLayout(device, &(WGPUBindGroupLayoutDescriptor){
            .entryCount = 1,
            .entries = &(WGPUBindGroupLayoutEntry) {
                .binding = 0,
                .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                .buffer = {
                    .type = WGPUBufferBindingType_Uniform,
                    .minBindingSize = sizeof(ShaderData)
                }
            }
        });
    WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor){
            .layout = bind_group_layout,
            .entryCount = 1,
            .entries = &(WGPUBindGroupEntry){
                .binding = 0,
                .buffer = uniform_buffer,
                .offset = 0,
                .size = sizeof(ShaderData)
            }
        });

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &(WGPUShaderModuleDescriptor){
            .label = "Shdader descriptor",
            .hintCount = 0,
            .hints = nullptr,
            .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
                .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
                .code = shader
            }
        });

    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(device, &(WGPUPipelineLayoutDescriptor){
            .bindGroupLayoutCount = 1,
            .bindGroupLayouts = &bind_group_layout
        });

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &(WGPURenderPipelineDescriptor){
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
                        .attributeCount = 1,
                        .attributes = (WGPUVertexAttribute[]) {
                            [0] = {
                                .shaderLocation = 1,
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = 0
                            }
                        },
                        .arrayStride = instance_data_size,
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
                    .format = surface_format,
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
            .layout = pipeline_layout
        });

    wgpuShaderModuleRelease(shader_module);

    WGPUBuffer vertex_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
           .size = sizeof(vertex_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });
    wgpuQueueWriteBuffer(queue, vertex_buffer, 0, vertex_data, sizeof(vertex_data));

    WGPUBuffer index_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
           .size = sizeof(index_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
        });
    wgpuQueueWriteBuffer(queue, index_buffer, 0, index_data, sizeof(index_data));

    WGPUBuffer instance_buffer = wgpuDeviceCreateBuffer(device, &(WGPUBufferDescriptor) {
           .size = sizeof(instance_data),
           .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
        });
    wgpuQueueWriteBuffer(queue, instance_buffer, 0, instance_data, sizeof(instance_data));


    ShaderData shader_data = { .time = 0.0f, .color = { 0.3f, 0.3f, 1.0f } };

    // MAIN LOOP
    while (!glfwWindowShouldClose(window))
    {
        if (window_resized)
        {
            glfwGetFramebufferSize(window, &shader_data.resolution[0], &shader_data.resolution[1]);
            configure_surface(device, surface, surface_format, shader_data.resolution[0], shader_data.resolution[1]);
            window_resized = false;
        }

        glfwPollEvents();
        f32 prev_time = shader_data.time;
        shader_data.time = (f32)glfwGetTime();
        f32 dt = shader_data.time - prev_time;

        (void)dt;

        shader_data.color[0] = sinf(shader_data.time) * 0.5f + 0.5f;
        wgpuQueueWriteBuffer(queue, uniform_buffer, 0, &shader_data, sizeof(shader_data));

        WGPUSurfaceTexture surface_texture;
        wgpuSurfaceGetCurrentTexture(surface, &surface_texture);

        WGPUTextureView surface_texture_view = wgpuTextureCreateView(surface_texture.texture, &(WGPUTextureViewDescriptor){
                .label = "Surface texture view",
                .format = wgpuTextureGetFormat(surface_texture.texture),
                .dimension = WGPUTextureViewDimension_2D,
                .mipLevelCount = 1,
                .arrayLayerCount = 1,
                .aspect = WGPUTextureAspect_All,
            });

        // Render stuff

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &(WGPUCommandEncoderDescriptor){ .label = "Command encoder"});

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

        wgpuRenderPassEncoderSetPipeline(render_pass, pipeline);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, vertex_buffer, 0, wgpuBufferGetSize(vertex_buffer));
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, instance_buffer, 0, wgpuBufferGetSize(instance_buffer));
        wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(index_buffer));
        wgpuRenderPassEncoderSetBindGroup(render_pass, 0, bind_group, 0, nullptr);
        wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, instance_count, 0, 0, 0);

        wgpuRenderPassEncoderEnd(render_pass);

        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);

        wgpuSurfacePresent(surface);
        wgpuTextureRelease(surface_texture.texture);
        wgpuTextureViewRelease(surface_texture_view);
    }


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

    return 0;
}
