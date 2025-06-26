#include <webgpu/webgpu.h>
#include <glfw/glfw3.h>
#include <glfw3webgpu.h>

#include <stdio.h>
#include <marrow/marrow.h>

typedef struct {
    WGPUAdapter adapter;
    bool request_done;
} RequestAdapterUserData;

void request_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata)
{
    if (status != WGPURequestAdapterStatus_Success)
    {
        abort("Failed to get WebGPU adapter");
    }

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

const char* shader = "\
@vertex \
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {\
    var p = vec2f(0.0, 0.0);\
    if (in_vertex_index == 0u){\
        p = vec2f(-0.5, -0.5);\
    } else if (in_vertex_index == 1u){\
        p = vec2f(0.5, -0.5);\
    } else {\
        p = vec2f(0.0, 0.5);\
    }\
    return vec4f(p, 0.0, 1.0);\
}\
\
@fragment \
fn fs_main() -> @location(0) vec4f {\
    return vec4f(0.0, 0.4, 1.0, 1.0);\
}\
";

int main(int argc, char* argv[])
{
    // SET UP GLFW
    if (!glfwInit())
        abort("Could not intialize GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 800, "HIIII HIII HIII!!!H IIIII HIH IHI HII!!!I HIIIII", nullptr, nullptr);
    if (!window)
        abort("Couldnt no open window!");

    // SET UP WGPU
    WGPUInstanceDescriptor desc = { 0 };
    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        abort("Failed to create WebGPU instance.");
        return 1;
    }

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);
    WGPUAdapter adapter = get_adapter(instance, (WGPURequestAdapterOptions){ .compatibleSurface = surface });
    if (!adapter)
        abort("Failed to get the adapter!");

    debug("Successfully got the adapter!");

    WGPUDevice device = get_device(adapter);

    if (!device)
        abort("Failed to get the device!");

    debug("Succesfully got the device!");

    WGPUTextureFormat surface_format = wgpuSurfaceGetPreferredFormat(surface, adapter);

    wgpuSurfaceConfigure(surface, &(WGPUSurfaceConfiguration){
            .width = 800,
            .height = 800,
            .format = surface_format,
            .usage = WGPUTextureUsage_RenderAttachment,
            .device = device,
            .presentMode = WGPUPresentMode_Fifo,
            .alphaMode = WGPUCompositeAlphaMode_Auto
        });

    wgpuAdapterRelease(adapter);

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    wgpuQueueOnSubmittedWorkDone(queue, &queue_on_submitted_work_done, nullptr);

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &(WGPUShaderModuleDescriptor){
            .label = "Shdader descriptor",
            .hintCount = 0,
            .hints = nullptr,
            .nextInChain = (WGPUChainedStruct*)&(WGPUShaderModuleWGSLDescriptor) {
                .chain.sType = WGPUSType_ShaderModuleWGSLDescriptor,
                .code = shader
            }
        });

    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &(WGPURenderPipelineDescriptor){
            .vertex = {
                .module = shader_module,
                .entryPoint = "vs_main"
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
            .layout = nullptr
        });

    wgpuShaderModuleRelease(shader_module);

    // MAIN LOOP
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

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
        wgpuCommandEncoderInsertDebugMarker(encoder, "First thing");
        wgpuCommandEncoderInsertDebugMarker(encoder, "Second thing");

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
        wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

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

    wgpuRenderPipelineRelease(pipeline);

    wgpuQueueRelease(queue);

    wgpuDeviceRelease(device);

    wgpuSurfaceUnconfigure(surface);

    wgpuInstanceRelease(instance);

    // CLEAN UP GLFW

    glfwDestroyWindow(window);

    return 0;
}
