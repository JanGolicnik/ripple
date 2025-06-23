#include <webgpu/webgpu.h>
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

int main(int argc, char* argv[])
{
    WGPUInstanceDescriptor desc = { 0 };
    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        abort("Failed to create WebGPU instance.");
        return 1;
    }

    WGPURequestAdapterOptions request_adapter_options = { 0 };
    RequestAdapterUserData request_adapter_user_data = { 0 };
    wgpuInstanceRequestAdapter(instance, &request_adapter_options, request_adapter_callback, &request_adapter_user_data);

    WGPUAdapter adapter = request_adapter_user_data.adapter;
    if (adapter)
    {
        debug("Succesfully got the adapter!");
    }

    WGPUSupportedLimits supported_limits = { 0 };
    if (wgpuAdapterGetLimits(adapter, &supported_limits))
    {
        printout("Adapter limits:\n"
                 " - maxTextureDimension1D: {}\n"
                 " - maxTextureDimension2D: {}\n"
                 " - maxTextureDimension3D: {}\n"
                 " - maxTextureArrayLayers: {}\n",
                 supported_limits.limits.maxTextureDimension1D,
                 supported_limits.limits.maxTextureDimension2D,
                 supported_limits.limits.maxTextureDimension3D,
                 supported_limits.limits.maxTextureArrayLayers
        );
    }

    usize n_features = wgpuAdapterEnumerateFeatures(adapter, nullptr);

    WGPUFeatureName features[n_features];
    wgpuAdapterEnumerateFeatures(adapter, features);

    printout("Adapter features:\n");
    for_each(feature, features, n_features)
    {
        printout(" - 0x{Xd}\n", *feature);
    }

    WGPUAdapterProperties properties = { 0 };
    wgpuAdapterGetProperties(adapter, &properties);

    printout("Adapter properties:\n"
             " - vendorID: {}\n"
             " - vendorName: {}\n"
             " - architecture: {}\n"
             " - deviceID: {}\n"
             " - name: {}\n"
             " - driverDescription: {}\n"
             " - adapterType: {Xd}\n"
             " - backendType: {Xd}\n",
             properties.vendorID,
             properties.vendorName ? properties.vendorName : "Unknown",
             properties.architecture ? properties.architecture : "Unknown",
             properties.deviceID,
             properties.name ? properties.name : "Unknown",
             properties.driverDescription ? properties.driverDescription : "Unknown",
             properties.adapterType,
             properties.backendType
    );

    wgpuAdapterRelease(adapter);

    wgpuInstanceRelease(instance);

    return 0;
}
