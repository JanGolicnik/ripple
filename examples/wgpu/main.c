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

    wgpuSurfaceConfigure(surface, &(WGPUSurfaceConfiguration){
            .width = 800,
            .height = 800,
            .format = wgpuSurfaceGetPreferredFormat(surface, adapter),
            .usage = WGPUTextureUsage_RenderAttachment,
            .device = device,
            .presentMode = WGPUPresentMode_Fifo,
            .alphaMode = WGPUCompositeAlphaMode_Auto
        });

    wgpuAdapterRelease(adapter);

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    wgpuQueueOnSubmittedWorkDone(queue, &queue_on_submitted_work_done, nullptr);

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

        wgpuRenderPassEncoderEnd(
            wgpuCommandEncoderBeginRenderPass(encoder, &(WGPURenderPassDescriptor){
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
            })
        );

        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &(WGPUCommandBufferDescriptor){ .label = "Command buffer" });
        wgpuCommandEncoderRelease(encoder);
        debug("Submitting command");
        wgpuQueueSubmit(queue, 1, &command);
        wgpuCommandBufferRelease(command);
        debug("Command submitted");

        wgpuSurfacePresent(surface);
        wgpuTextureRelease(surface_texture.texture);
        wgpuTextureViewRelease(surface_texture_view);
    }


    // CLEAN UP WGPU

    wgpuQueueRelease(queue);

    wgpuDeviceRelease(device);

    wgpuSurfaceUnconfigure(surface);

    wgpuInstanceRelease(instance);

    // CLEAN UP GLFW

    glfwDestroyWindow(window);

    return 0;
}
