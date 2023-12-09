#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <webgpu.h>
#include <wgpu.h>

#include <iostream>
#include <vector>
#include <chrono>

const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

WGPUSurface SDL_WGPU_CreateSurface(WGPUInstance instance, SDL_Window * window)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(window, &info);

    switch (info.subsystem)
    {
#ifdef SDL_VIDEO_DRIVER_X11
    case SDL_SYSWM_X11:
        {
            WGPUSurfaceDescriptorFromXlibWindow surfaceDescriptorFromXlibWindow;
            surfaceDescriptorFromXlibWindow.chain.next = nullptr;
            surfaceDescriptorFromXlibWindow.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
            surfaceDescriptorFromXlibWindow.window = info.info.x11.window;
            surfaceDescriptorFromXlibWindow.display = info.info.x11.display;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = nullptr;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromXlibWindow;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#elif SDL_VIDEO_DRIVER_WAYLAND
    case SDL_SYSWM_X11:
        {
            WGPUSurfaceDescriptorFromWaylandSurface surfaceDescriptorFromWaylandSurface;
            surfaceDescriptorFromWaylandSurface.chain.next = nullptr;
            surfaceDescriptorFromWaylandSurface.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
            surfaceDescriptorFromWaylandSurface.display = info.info.wl.display;
            surfaceDescriptorFromWaylandSurface.surface = info.info.wl.surface;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = nullptr;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWaylandSurface;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

#ifdef SDL_VIDEO_DRIVER_WINDOWS
    case SDL_SYSWM_WINDOWS:
        {
            WGPUSurfaceDescriptorFromWindowsHWND surfaceDescriptorFromWindowsHWND;
            surfaceDescriptorFromWindowsHWND.chain.next = nullptr;
            surfaceDescriptorFromWindowsHWND.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
            surfaceDescriptorFromWindowsHWND.hwnd = info.info.win.window;
            surfaceDescriptorFromWindowsHWND.hinstance = info.info.win.hinstance;

            WGPUSurfaceDescriptor surfaceDescriptor;
            surfaceDescriptor.label = nullptr;
            surfaceDescriptor.nextInChain = (const WGPUChainedStruct*)&surfaceDescriptorFromWindowsHWND;

            return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
        }
#endif

        // TODO: support MacOS
    default:
        throw std::runtime_error("System not supported");
    }

}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);

    int width = 1024;
    int height = 768;

    auto window = SDL_CreateWindow("WebGPU Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    WGPUInstanceDescriptor instanceDescriptor;
    instanceDescriptor.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&instanceDescriptor);

    std::cout << "Instance: " << instance << std::endl;

    WGPUSurface surface = SDL_WGPU_CreateSurface(instance, window);

    std::cout << "Surface: " << surface << std::endl;

    WGPURequestAdapterOptions requestAdapterOptions;
    requestAdapterOptions.nextInChain = nullptr;
    requestAdapterOptions.compatibleSurface = surface;
    requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;
    requestAdapterOptions.backendType = WGPUBackendType_Vulkan;
    requestAdapterOptions.forceFallbackAdapter = false;

    WGPUAdapter adapter = nullptr;

    auto requestAdapterCallback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * userdata){
        if (message)
            std::cout << "Adapter callback message: " << message << std::endl;

        if (status == WGPURequestAdapterStatus_Success)
            *(WGPUAdapter *)(userdata) = adapter;
        else
            throw std::runtime_error(message);
    };

    wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback, &adapter);

    std::cout << "Adapter: " << adapter << std::endl;

    if (!adapter)
        throw std::runtime_error("Adapter not created");

    std::vector<WGPUFeatureName> features;
    features.resize(wgpuAdapterEnumerateFeatures(adapter, nullptr));
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    for (auto const & feature : features)
        std::cout << "    " << feature << std::endl;

    WGPUSupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supportedLimits);

    std::cout << "Supported limits:" << std::endl;
    std::cout << "    maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
    std::cout << "    maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
    std::cout << "    maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
    std::cout << "    maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    std::cout << "    maxBindGroups: " << supportedLimits.limits.maxBindGroups << std::endl;
    std::cout << "    maxBindGroupsPlusVertexBuffers: " << supportedLimits.limits.maxBindGroupsPlusVertexBuffers << std::endl;
    std::cout << "    maxBindingsPerBindGroup: " << supportedLimits.limits.maxBindingsPerBindGroup << std::endl;
    std::cout << "    maxDynamicUniformBuffersPerPipelineLayout: " << supportedLimits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
    std::cout << "    maxDynamicStorageBuffersPerPipelineLayout: " << supportedLimits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
    std::cout << "    maxSampledTexturesPerShaderStage: " << supportedLimits.limits.maxSampledTexturesPerShaderStage << std::endl;
    std::cout << "    maxSamplersPerShaderStage: " << supportedLimits.limits.maxSamplersPerShaderStage << std::endl;
    std::cout << "    maxStorageBuffersPerShaderStage: " << supportedLimits.limits.maxStorageBuffersPerShaderStage << std::endl;
    std::cout << "    maxStorageTexturesPerShaderStage: " << supportedLimits.limits.maxStorageTexturesPerShaderStage << std::endl;
    std::cout << "    maxUniformBuffersPerShaderStage: " << supportedLimits.limits.maxUniformBuffersPerShaderStage << std::endl;
    std::cout << "    maxUniformBufferBindingSize: " << supportedLimits.limits.maxUniformBufferBindingSize << std::endl;
    std::cout << "    maxStorageBufferBindingSize: " << supportedLimits.limits.maxStorageBufferBindingSize << std::endl;
    std::cout << "    minUniformBufferOffsetAlignment: " << supportedLimits.limits.minUniformBufferOffsetAlignment << std::endl;
    std::cout << "    minStorageBufferOffsetAlignment: " << supportedLimits.limits.minStorageBufferOffsetAlignment << std::endl;
    std::cout << "    maxVertexBuffers: " << supportedLimits.limits.maxVertexBuffers << std::endl;
    std::cout << "    maxBufferSize: " << supportedLimits.limits.maxBufferSize << std::endl;
    std::cout << "    maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
    std::cout << "    maxVertexBufferArrayStride: " << supportedLimits.limits.maxVertexBufferArrayStride << std::endl;
    std::cout << "    maxInterStageShaderComponents: " << supportedLimits.limits.maxInterStageShaderComponents << std::endl;
    std::cout << "    maxInterStageShaderVariables: " << supportedLimits.limits.maxInterStageShaderVariables << std::endl;
    std::cout << "    maxColorAttachments: " << supportedLimits.limits.maxColorAttachments << std::endl;
    std::cout << "    maxColorAttachmentBytesPerSample: " << supportedLimits.limits.maxColorAttachmentBytesPerSample << std::endl;
    std::cout << "    maxComputeWorkgroupStorageSize: " << supportedLimits.limits.maxComputeWorkgroupStorageSize << std::endl;
    std::cout << "    maxComputeInvocationsPerWorkgroup: " << supportedLimits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
    std::cout << "    maxComputeWorkgroupSizeX: " << supportedLimits.limits.maxComputeWorkgroupSizeX << std::endl;
    std::cout << "    maxComputeWorkgroupSizeY: " << supportedLimits.limits.maxComputeWorkgroupSizeY << std::endl;
    std::cout << "    maxComputeWorkgroupSizeZ: " << supportedLimits.limits.maxComputeWorkgroupSizeZ << std::endl;
    std::cout << "    maxComputeWorkgroupsPerDimension: " << supportedLimits.limits.maxComputeWorkgroupsPerDimension << std::endl;

    WGPURequiredLimits requiredLimits;
    requiredLimits.nextInChain = nullptr;
    requiredLimits.limits = supportedLimits.limits;

    WGPUDeviceDescriptor deviceDescriptor;
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = nullptr;
    deviceDescriptor.requiredFeatureCount = 0;
    deviceDescriptor.requiredFeatures = nullptr;
    deviceDescriptor.requiredLimits = &requiredLimits;
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = nullptr;
    deviceDescriptor.deviceLostCallback = nullptr;
    deviceDescriptor.deviceLostUserdata = nullptr;

    WGPUDevice device;

    auto requestDeviceCallback = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * userdata)
    {
        if (message)
            std::cout << "Device callback message: " << message << std::endl;

        if (status == WGPURequestDeviceStatus_Success)
            *(WGPUDevice *)(userdata) = device;
        else
            throw std::runtime_error(message);
    };

    wgpuAdapterRequestDevice(adapter, &deviceDescriptor, requestDeviceCallback, &device);

    std::cout << "Device: " << device << std::endl;

    if (!device)
        throw std::runtime_error("Device not created");

    auto surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);

    std::cout << "Surface format: " << surfaceFormat << std::endl;

    WGPUSurfaceConfiguration surfaceConfiguration;
    surfaceConfiguration.nextInChain = nullptr;
    surfaceConfiguration.device = device;
    surfaceConfiguration.format = surfaceFormat;
    surfaceConfiguration.usage = WGPUTextureUsage_RenderAttachment;
    surfaceConfiguration.viewFormatCount = 1;
    surfaceConfiguration.viewFormats = &surfaceFormat;
    surfaceConfiguration.alphaMode = WGPUCompositeAlphaMode_Auto;
    surfaceConfiguration.width = width;
    surfaceConfiguration.height = height;
    surfaceConfiguration.presentMode = WGPUPresentMode_Fifo;

    wgpuSurfaceConfigure(surface, &surfaceConfiguration);

    WGPUQueue queue = wgpuDeviceGetQueue(device);

        WGPUShaderModuleDescriptor shaderDesc{};
        shaderDesc.nextInChain = nullptr;
        // [...] Describe shader module
        #ifdef WEBGPU_BACKEND_WGPU
        shaderDesc.hintCount = 0;
        shaderDesc.hints = nullptr;
        #endif


        WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
        // Set the chained struct's header
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        // Connect the chain
        shaderDesc.nextInChain = &shaderCodeDesc.chain;
        shaderCodeDesc.code = shaderSource;

        WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);


        WGPURenderPipelineDescriptor pipelineDesc{};
        pipelineDesc.nextInChain = nullptr;
        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;


        pipelineDesc.vertex.module = shaderModule;
        pipelineDesc.vertex.entryPoint = "vs_main";
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;

        // Each sequence of 3 vertices is considered as a triangle
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;

        // We'll see later how to specify the order in which vertices should be
        // connected. When not specified, vertices are considered sequentially.
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;

        // The face orientation is defined by assuming that when looking
        // from the front of the face, its corner vertices are enumerated
        // in the counter-clockwise (CCW) order.
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;

        // But the face orientation does not matter much because we do not
        // cull (i.e. "hide") the faces pointing away from us (which is often
        // used for optimization).
        pipelineDesc.primitive.cullMode = WGPUCullMode_None;

        WGPUFragmentState fragmentState{};
        fragmentState.nextInChain = nullptr;
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.constants = nullptr;
        // [...] We'll configure the blend stage here
        pipelineDesc.fragment = &fragmentState;

        pipelineDesc.depthStencil = nullptr; // Ignore depth stencil for now

        WGPUBlendState blendState{};
        // [...] Configure blend state

        WGPUColorTargetState colorTarget{};
        colorTarget.nextInChain = nullptr;
        colorTarget.format = surfaceFormat;
        colorTarget.blend = &blendState;
        colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation = WGPUBlendOperation_Add;

        blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;

        // We have only one target because our render pass has only one output color
        // attachment.
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTarget;

        // Samples per pixel
        pipelineDesc.multisample.count = 1;
        // Default value for the mask, meaning "all bits on"
        pipelineDesc.multisample.mask = ~0u;
        // Default value as well (irrelevant for count = 1 anyways)
        pipelineDesc.multisample.alphaToCoverageEnabled = false;

        pipelineDesc.layout = nullptr;

        WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
        std::cout << "Render pipeline: " << pipeline << std::endl;

    int frame = 0;

    for (bool running = true; running;)
    {
        std::cout << "Frame " << frame << std::endl;

        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        }

        WGPUSurfaceTexture surfaceTexture;

        auto getTextureStart = std::chrono::high_resolution_clock::now();

        wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated)
        {
            SDL_GetWindowSize(window, &width, &height);
            std::cout << "Resized to " << width << "x" << height << std::endl;

            surfaceConfiguration.width = width;
            surfaceConfiguration.height = height;
            wgpuSurfaceConfigure(surface, &surfaceConfiguration);

            wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
        }

        if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Timeout)
        {
            std::cout << "Timeout" << std::endl;
            ++frame;
            continue;
        }

        auto getTextureEnd = std::chrono::high_resolution_clock::now();
        std::cout << "Waited " << (std::chrono::duration_cast<std::chrono::duration<double>>(getTextureEnd - getTextureStart).count() * 1000.0) << " ms for the swapchain image" << std::endl;

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success)
            throw std::runtime_error("Can't get surface texture: " + std::to_string(surfaceTexture.status));

        WGPUTextureViewDescriptor textureViewDescriptor;
        textureViewDescriptor.nextInChain = nullptr;
        textureViewDescriptor.label = nullptr;
        textureViewDescriptor.format = surfaceFormat;
        textureViewDescriptor.dimension = WGPUTextureViewDimension_2D;
        textureViewDescriptor.baseMipLevel = 0;
        textureViewDescriptor.mipLevelCount = 1;
        textureViewDescriptor.baseArrayLayer = 0;
        textureViewDescriptor.arrayLayerCount = 1;
        textureViewDescriptor.aspect = WGPUTextureAspect_All;
        WGPUTextureView surfaceTextureView = wgpuTextureCreateView(surfaceTexture.texture, &textureViewDescriptor);

        WGPUCommandEncoderDescriptor commandEncoderDescriptor;
        commandEncoderDescriptor.nextInChain = nullptr;
        commandEncoderDescriptor.label = "Command Encoder";

        WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDescriptor);

        WGPURenderPassColorAttachment renderPassColorAttachment;
        // renderPassColorAttachment.nextInChain = nullptr;
        renderPassColorAttachment.view = surfaceTextureView;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = {1.0, 0.0, 1.0, 1.0};

        WGPURenderPassDescriptor renderPassDescriptor;
        renderPassDescriptor.nextInChain = nullptr;
        renderPassDescriptor.label = nullptr;
        renderPassDescriptor.colorAttachmentCount = 1;
        renderPassDescriptor.colorAttachments = &renderPassColorAttachment;
        renderPassDescriptor.depthStencilAttachment = nullptr;
        WGPUQuerySet occlusionQuerySet = nullptr;
        renderPassDescriptor.timestampWrites = nullptr;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(commandEncoder, &renderPassDescriptor);

        // Select which render pipeline to use
        wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
        // Draw 1 instance of a 3-vertices shape
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(renderPass);

        wgpuTextureViewRelease(surfaceTextureView);

        WGPUCommandBufferDescriptor commandBufferDescriptor;
        commandBufferDescriptor.nextInChain = nullptr;
        commandBufferDescriptor.label = nullptr;

        WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(commandEncoder, &commandBufferDescriptor);
        wgpuQueueSubmit(queue, 1, &commandBuffer);

        wgpuSurfacePresent(surface);

        ++frame;
    }
}
