#include "dx12_video.h"

#include "dx12_descriptor_pool.h"
#include "dx12_painter.h"
#include "dx12_pipeline.h"
#include "dx12_render_target.h"
#include "dx12_resource.h"
#include "dx12_shader.h"

#if defined(_GAMING_XBOX_XBOXONE)
#pragma comment (lib, "d3d12_x.lib")
#elif defined(_GAMING_XBOX_SCARLETT)
#pragma comment (lib, "d3d12_xs.lib")
#else
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#endif

#ifdef D3D12_DEBUG
#pragma comment( lib, "dxguid.lib")
#endif

using namespace Halley;

bool DX12Video::initialized = false;

DX12Video::DX12Video(SystemAPI &system)
    : system(system)
{

}

DX12Video::~DX12Video() = default;

void DX12Video::init()
{
    initialized = true;
    loader = std::make_unique<DX12Loader>(system);

#ifdef WITH_GDK
	if (RegisterAppStateChangeNotification([](BOOLEAN quiesced, PVOID context)
	{
        DX12Video* self = static_cast<DX12Video*>(context);

		Concurrent::execute(Executors::getMainUpdateThread(), [self, quiesced]
		{
            // TODO:
            // This should actually be done by Core, but those are not called
            // anywhere right now.

			if (quiesced) {
                self->onSuspend();
            } else {
	            self->onResume();
            }
		}).wait();
	}, this, &plmHandle)) {
		Logger::logWarning("Couldn't register PLM state change notification");
	}
#endif
}

void DX12Video::deInit()
{
#ifdef WITH_GDK
	UnregisterAppStateChangeNotification(plmHandle);
#endif

    if (initialized && device != nullptr) {
		waitForGpu();
	    shutdownDeviceAndSwapChain();
	}
    loader.reset();
    initialized = false;
}

bool DX12Video::isInitialized()
{
    return initialized;
}

void DX12Video::onResume()
{
#ifdef _GAMING_XBOX
    {
		std::unique_lock lock(commandQueueLock);
	    commandQueue->ResumeX();
    }
#endif

	loader = std::make_unique<DX12Loader>(system);

    registerFrameEvents();

    suspended = false;
}

void DX12Video::onSuspend()
{
    loader.reset();

#ifdef _GAMING_XBOX
    {
		std::unique_lock lock(commandQueueLock);
	    commandQueue->SuspendX(0);
    }
#endif

    suspended = true;
}

std::unique_ptr<Painter> DX12Video::makePainter(Resources &resources)
{
    return std::make_unique<DX12Painter>(*this, resources);
}

void DX12Video::startRender()
{
    if (suspended) {
	    return;
    }

#ifdef _GAMING_XBOX
    framePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;

    device->WaitFrameEventX(
            D3D12XBOX_FRAME_EVENT_ORIGIN,
            D3D12XBOX_WAIT_FRAME_EVENT_INFINITE,
            nullptr,
            D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE,
            &framePipelineToken
    );
#endif

    const Vector2i windowSize = window->getWindowRect().getSize();

    resizeSwapChain(windowSize);
    doDeferredResourceUpdates(false);

    auto frame = &frames[currentFrameIndex];

    frame->commandAllocator->Reset();
    commandList->Reset(frame->commandAllocator.Get(), nullptr);

    /*
     * Transition back buffer into "render-target" state.
     */
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = frame->backBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);
    }

    /*
     * Clear color and depth buffers.
     */
    {
        float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvPool->getCPUHandle(frame->backBufferDescriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvPool->getCPUHandle(depthBufferDescriptor);

        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    /*
     * Default a few render states.
     */
    {
        const D3D12_VIEWPORT viewport = {0, 0, float(windowSize.x), float(windowSize.y), 0.0f, 1.0f};
        commandList->RSSetViewports(1, &viewport);

        const D3D12_RECT scissor = {0, 0, windowSize.x, windowSize.y};
        commandList->RSSetScissorRects(1, &scissor);

        ID3D12DescriptorHeap* heaps[] = {srvPool->getHeap(), samPool->getHeap()};
        commandList->SetDescriptorHeaps(2, heaps);

        constantBuffers[currentFrameIndex]->resetData();
    }
}

void DX12Video::finishRender()
{
    if (suspended) {
	    return;
    }

    auto frame = &frames[currentFrameIndex];

    /*
     * Transition back buffer into "present" state.
     */
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = frame->backBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        commandList->ResourceBarrier(1, &barrier);
    }

    /*
     * Close and execute the command list(s).
     */
    {
        commandList->Close();

        ID3D12CommandList *commandLists[1] = {commandList.Get()};

		std::unique_lock lock(commandQueueLock);
        commandQueue->ExecuteCommandLists(1, commandLists);
    }

    /*
     * Present frame (flip).
     */
    {
#ifdef _GAMING_XBOX
        D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
        planeParameters.Token = framePipelineToken;
        planeParameters.ResourceCount = 1;
        planeParameters.ppResources = frame->backBuffer.GetAddressOf();

		std::unique_lock lock(commandQueueLock);
        commandQueue->PresentX(1, &planeParameters, nullptr);
#else
        uint32_t syncInterval = (allowTearing && !useVSync) ? 0u : 1u;
        uint32_t presentFlags = (allowTearing && !useVSync) ? DXGI_PRESENT_ALLOW_TEARING : 0u;

        swapChain->Present(syncInterval, presentFlags);
#endif
    }

    moveToNextFrame();
}

ID3D12GraphicsCommandList* DX12Video::startUpload()
{
    loaderCommandAllocator->Reset();
    loaderCommandList->Reset(loaderCommandAllocator.Get(), nullptr);
    return loaderCommandList.Get();
}

void DX12Video::finishUpload()
{
    loaderCommandList->Close();
    ID3D12CommandList* commandLists[1] = {loaderCommandList.Get()};

	{
		std::unique_lock lock(commandQueueLock);
		commandQueue->ExecuteCommandLists(1, commandLists);

	    if (FAILED(commandQueue->Signal(loaderFence.Get(), loaderFenceValue + 1))) {
            throw Exception("Signal() failed", HalleyExceptions::VideoPlugin);
	    }
	}

    if (SUCCEEDED(loaderFence->SetEventOnCompletion(loaderFenceValue + 1, loaderFenceEvent))) {
        WaitForSingleObjectEx(loaderFenceEvent, INFINITE, false);
    }

    loaderFenceValue++;
}

void DX12Video::setWindow(WindowDefinition&& windowDescriptor)
{
    if (!window) {
        window = system.createWindow(windowDescriptor);
        initDeviceAndSwapChain();
    } else {
        window->update(windowDescriptor);
    }
}

Window& DX12Video::getWindow() const
{
    return *window;
}

bool DX12Video::hasWindow() const
{
    return window != nullptr;
}

void DX12Video::setVsync(bool vsync)
{
#ifdef _GAMING_XBOX
    useVSync = true;
#else
    useVSync = vsync;
#endif
}

bool DX12Video::hasVsync() const
{
    return useVSync;
}

void DX12Video::waitForVsync()
{
    // TODO
}

void DX12Video::flush() {
    waitForGpu();
    //doDeferredResourceUpdates();
}

std::unique_ptr<Texture> DX12Video::createTexture(Vector2i size)
{
    return std::make_unique<DX12Texture>(*this, size);
}

std::unique_ptr<Shader> DX12Video::createShader(const ShaderDefinition &definition)
{
    return std::make_unique<DX12Shader>(*this, definition);
}

std::unique_ptr<TextureRenderTarget> DX12Video::createTextureRenderTarget()
{
    return std::make_unique<DX12TextureRenderTarget>(*this);
}

std::unique_ptr<ScreenRenderTarget> DX12Video::createScreenRenderTarget()
{
    auto view = Rect4i(Vector2i(), swapChainRenderSize);
    return std::make_unique<DX12ScreenRenderTarget>(*this, view);
}

void DX12Video::setScreenRenderTarget()
{
    auto frame = &frames[currentFrameIndex];

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvPool->getCPUHandle(frame->backBufferDescriptor);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvPool->getCPUHandle(depthBufferDescriptor);

    commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Video::getScreenRenderTargetView() const
{
    auto frame = &frames[currentFrameIndex];
    return rtvPool->getCPUHandle(frame->backBufferDescriptor);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Video::getScreenDepthStencilView() const
{
    return dsvPool->getCPUHandle(depthBufferDescriptor);
}

std::unique_ptr<MaterialConstantBuffer> DX12Video::createConstantBuffer()
{
    return std::make_unique<DX12MaterialConstantBuffer>(*this);
}

String DX12Video::getShaderLanguage()
{
#if defined(_GAMING_XBOX_XBOXONE)
    return "dxbc_x";
#elif defined(_GAMING_XBOX_SCARLETT)
    return "dxbc_xs";
#else
    return "dxil";
#endif
}

void* DX12Video::getImplementationPointer(const String &id)
{
#if !defined(_GAMING_XBOX)
    if (id == "ID3D12Device") {
        return static_cast<IUnknown*>(device.Get());
    }
#endif
    return nullptr;
}

void DX12Video::initDeviceAndSwapChain()
{
#if D3D12_DEBUG
    /*
     * Try to enable debug layers. Any failures are ignored.
     */
    ComPtr<ID3D12Debug> debugInterface;
    if (SUCCEEDED(D3D12GetDebugInterface(D3D12_IID_ARGS(debugInterface)))) {
        debugInterface->EnableDebugLayer();
        /*
         * ID3D12Debug1 doesn't exist on XboxOne.
         */
        ComPtr<ID3D12Debug1> debugInterface1;
        if (SUCCEEDED(debugInterface.As(&debugInterface1))) {
            debugInterface1->SetEnableGPUBasedValidation(true);
        }
    }
#endif

#ifdef _GAMING_XBOX
    /*
     * No use of DXGI on consoles. Just call D3D12XboxCreateDevice().
     */
    {
        D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
        params.Version = D3D12_SDK_VERSION;
        params.GraphicsCommandQueueRingSizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
		params.GraphicsScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;
		params.ComputeScratchMemorySizeBytes = D3D12XBOX_DEFAULT_SIZE_BYTES;

#if D3D12_DEBUG
        params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#endif

        params.DisableGeometryShaderAllocations = TRUE;
        params.DisableTessellationShaderAllocations = TRUE;

        //params.ProcessDebugFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_ENABLE_COMMON_STATE_PROMOTION;

        HRESULT hr = D3D12XboxCreateDevice(
                nullptr,
                &params,
                IID_GRAPHICS_PPV_ARGS(device.ReleaseAndGetAddressOf())
        );

        if (hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH) {
            throw Exception("d3d12_x.lib version mismatch", HalleyExceptions::VideoPlugin);
        }

        if (FAILED(hr)) {
            throw Exception("Failed to create D3D12Xbox device", HalleyExceptions::VideoPlugin);
        }
    }
#else
    /*
     * Create DXGI factory, and enumerate DXGI adapters available.
     */
    {
        UINT flags = D3D12_DEBUG ? DXGI_CREATE_FACTORY_DEBUG : 0u;
        if (FAILED(CreateDXGIFactory2(flags, D3D12_IID_ARGS(dxgiFactory)))) {
            throw Exception("Failed to create DXGI factory", HalleyExceptions::VideoPlugin);
        }
    }

    /*
     * Probe adapters with IDXGIFactory6::EnumAdapters1() until
     * DXGI_ERROR_NOT_FOUND is returned.
     *
     * Software adapters are ignored. For any hardware adapters, the code
     * simply selects the one with the most dedicated video RAM.
     */
    {
        HRESULT hr = 0;
        for (UINT adapterIndex = 0; SUCCEEDED(hr); adapterIndex++) {
            ComPtr<IDXGIAdapter1> adapter1;

            hr = dxgiFactory->EnumAdapters1(adapterIndex, &adapter1);
            Ensures(SUCCEEDED(hr) || (hr == DXGI_ERROR_NOT_FOUND));

            if (FAILED(hr)) {
                continue;
            }

            DXGI_ADAPTER_DESC1 desc;
            if (FAILED(adapter1->GetDesc1(&desc))) {
                continue;
            }

            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
                continue;
            }

            /*
             * Dry-run device creation, using the minimum feature level.
             */
            if (FAILED(D3D12CreateDevice(
                    adapter1.Get(),
                    D3D_FEATURE_LEVEL_12_0,
                    __uuidof(ID3D12Device),
                    nullptr))) {
                continue;
            }

            if (adapterDesc.DedicatedVideoMemory < desc.DedicatedVideoMemory) {
                adapter1.As(&adapter);
                adapterDesc = desc;
            }
        }

        if (adapter == nullptr) {
            throw Exception("No suitable DXGI adapter found", HalleyExceptions::VideoPlugin);
        }
    }

    /*
     * Now create the device "for real".
     */
    if (FAILED(D3D12CreateDevice(
            adapter.Get(),
            D3D_FEATURE_LEVEL_12_0,
            D3D12_IID_ARGS(device)))) {
        throw Exception("Failed to create D3D12 device", HalleyExceptions::VideoPlugin);
    }

#if D3D12_DEBUG
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(device.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            D3D12_MESSAGE_SEVERITY denySeverities[] = {
                    D3D12_MESSAGE_SEVERITY_INFO,
            };

            D3D12_MESSAGE_ID denyIds[] = {
                    D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
                    D3D12_MESSAGE_ID_CREATEBLENDSTATE_BLENDOP_WARNING,
                    /* Triggered by Steam overlay. Yes, really! */
					D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumSeverities = _countof(denySeverities);
            filter.DenyList.pSeverityList = denySeverities;
            filter.DenyList.NumIDs = _countof(denyIds);
            filter.DenyList.pIDList = denyIds;

            infoQueue->PushStorageFilter(&filter);
        }

        DXGIGetDebugInterface1(0, D3D12_IID_ARGS(dxgiDebug));
    }
#endif

    /*
     * Check for "tearing" support (G-Sync, FreeSync).
     */
    {
        BOOL feature = FALSE;
        if (SUCCEEDED(dxgiFactory->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &feature,
                sizeof(feature)))) {
            allowTearing = feature;
        } else {
            allowTearing = false;
        }
    }
#endif

    /*
     * The command queue.
     */
    createCommandQueue(commandQueue, D3D12_COMMAND_LIST_TYPE_DIRECT);

    /*
     * Descriptor heaps for shader-resource, render-target and
     * depth-stencil views.
     */
    srvPool = std::make_unique<DX12DescriptorPool>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048);
    samPool = std::make_unique<DX12DescriptorPool>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
    rtvPool = std::make_unique<DX12DescriptorPool>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128);
    dsvPool = std::make_unique<DX12DescriptorPool>(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);

    /*
     * Synchronization primitives (fence) for the main render loop.
     */
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, D3D12_IID_ARGS(fence)))) {
        throw Exception("Failed to create fence", HalleyExceptions::VideoPlugin);
    }

    fenceEvent = CreateEvent(nullptr, false, false, nullptr);

#if defined(_GAMING_XBOX)
    /*
     * Default to hard-coded render size on console.
     */
    switch(XSystemGetDeviceType()) {
    case XSystemDeviceType::XboxOne:
    case XSystemDeviceType::XboxOneS:
        swapChainRenderSize.x = 1920;
        swapChainRenderSize.y = 1080;
        break;
    case XSystemDeviceType::XboxScarlettLockhart:
        swapChainRenderSize.x = 2560;
        swapChainRenderSize.y = 1440;
        break;
    case XSystemDeviceType::XboxOneX:
    case XSystemDeviceType::XboxOneXDevkit:
    case XSystemDeviceType::XboxScarlettAnaconda:
    case XSystemDeviceType::XboxScarlettDevkit:
        swapChainRenderSize.x = 3840;
        swapChainRenderSize.y = 2160;
        break;
    case XSystemDeviceType::Unknown:
    case XSystemDeviceType::Pc:
        break;
    }
#else
    /*
     * Create the DXGI swap chain.
     */
    {
        swapChainRenderSize = window->getWindowRect().getSize();

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = (uint32_t) swapChainRenderSize.x;
        desc.Height = (uint32_t) swapChainRenderSize.y;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Stereo = false;
        desc.SampleDesc = {1, 0};
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = numFrames;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        HWND hwnd = (HWND) window->getNativeHandle();

        ComPtr<IDXGISwapChain1> swapChain1;
        if (FAILED(dxgiFactory->CreateSwapChainForHwnd(
                commandQueue.Get(),
                hwnd,
                &desc,
                nullptr,
                nullptr,
                &swapChain1))) {
            throw Exception("Failed to create swap chain", HalleyExceptions::VideoPlugin);
        }

        dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

        swapChain1.As(&swapChain);
    }
#endif

    /*
     * Initial setup of render target views.
     */
    updateRenderTargetViews();

    /*
     * Per-frame command allocator and command list.
     */
    for (uint32_t i = 0; i < numFrames; i++) {
        createCommandAllocator(frames[i].commandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    createCommandList(commandList, frames[0].commandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    /*
     * Resources for uploading constant buffer data.
     */
    for (uint32_t i = 0; i < numFrames; i++) {
        constantBuffers[i] = std::make_unique<DX12Buffer>(*this, DX12Buffer::Type::Constant, 65536);
    }

    /*
     * A second command list, allocator and transient buffer for uploading resources.
     */
    createCommandAllocator(loaderCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    createCommandList(loaderCommandList, loaderCommandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    size_t transientBufferSize = 4096 * 4096 * 4;
    loaderTransientBuffer = std::make_unique<DX12Buffer>(*this, DX12Buffer::Type::Upload, transientBufferSize);

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, D3D12_IID_ARGS(loaderFence)))) {
        throw Exception("Failed to create loader fence", HalleyExceptions::VideoPlugin);
    }

    loaderFenceEvent = CreateEvent(nullptr, false, false, nullptr);

    registerFrameEvents();
}

void DX12Video::shutdownDeviceAndSwapChain()
{
    doDeferredResourceUpdates(true);

#if D3D12_DEBUG && !defined(_GAMING_XBOX)
    /*
     * Stop hitting debug breakpoints here. We try to report leaks down below.
     */
    {
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(device.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
        }
    }
#endif

    CloseHandle(loaderFenceEvent);
    loaderFence.Reset();
    loaderTransientBuffer.reset();
    loaderCommandList.Reset();
    loaderCommandAllocator.Reset();

    CloseHandle(fenceEvent);
    fence.Reset();

    depthBuffer.Reset();

    commandList.Reset();

    for (uint32_t i = 0; i < numFrames; i++) {
        constantBuffers[i].reset();
    }

    dsvPool->free(depthBufferDescriptor);

    for (uint32_t i = 0; i < numFrames; i++) {
        frames[i].commandAllocator.Reset();
        rtvPool->free(frames[i].backBufferDescriptor);
        frames[i].backBuffer.Reset();
    }

    srvPool.reset();
    samPool.reset();
    rtvPool.reset();
    dsvPool.reset();

    commandQueue.Reset();
    device.Reset();

#if !defined(_GAMING_XBOX)
    swapChain.Reset();
    adapter.Reset();
    dxgiFactory.Reset();

    /*
     * Uses the DXGI debug interface, instead of the D3D12 debug interface,
     * because (as I suspect) the latter adds a reference to its device, and
     * then reports a leak about itself when reporting live objects.
     */
#if D3D12_DEBUG
    if (dxgiDebug) {
        /* Not exported by dxgi.lib */
        static const GUID D3D12_DXGI_DEBUG_ALL = {0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8}};
        auto flags = (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
        dxgiDebug->ReportLiveObjects(D3D12_DXGI_DEBUG_ALL, flags);
        dxgiDebug.Reset();
    }
#endif
#endif
}

void DX12Video::createCommandQueue(ComPtr<ID3D12CommandQueue>& queue, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    if (FAILED(device->CreateCommandQueue(&desc, D3D12_IID_ARGS(queue)))) {
        throw Exception("Failed to create command queue", HalleyExceptions::VideoPlugin);
    }
}

void DX12Video::createCommandAllocator(ComPtr<ID3D12CommandAllocator>& allocator, D3D12_COMMAND_LIST_TYPE type)
{
    if (FAILED(device->CreateCommandAllocator(type, D3D12_IID_ARGS(allocator)))) {
        throw Exception("Failed to create command allocator", HalleyExceptions::VideoPlugin);
    }
}

void DX12Video::createCommandList(ComPtr<ID3D12GraphicsCommandList>& list, const ComPtr<ID3D12CommandAllocator>& allocator, D3D12_COMMAND_LIST_TYPE type)
{
    if (FAILED(device->CreateCommandList(
            0, type, allocator.Get(), nullptr, D3D12_IID_ARGS(list)))) {
        throw Exception("Failed to create command list", HalleyExceptions::VideoPlugin);
    }
    list->Close();
}

void DX12Video::resizeSwapChain(const Vector2i& windowSize)
{
    if (windowSize.x == 0 || windowSize.y == 0) {
        return;
    }

    if (windowSize == swapChainRenderSize) {
        return;
    }

#if !defined(_GAMING_XBOX)
    swapChainRenderSize = windowSize;

    waitForGpu();

    for (uint32_t i = 0; i < numFrames; i++) {
        rtvPool->free(frames[i].backBufferDescriptor);
        frames[i].backBufferDescriptor = InvalidDescriptorHandle;

        frames[i].backBuffer.Reset();
        frames[i].fenceValue = frames[currentFrameIndex].fenceValue;
    }

    dsvPool->free(depthBufferDescriptor);
    depthBufferDescriptor = InvalidDescriptorHandle;

    depthBuffer.Reset();

    DXGI_SWAP_CHAIN_DESC desc = {};
    if (FAILED(swapChain->GetDesc(&desc))) {
        throw Exception("Failed to get swap chain description", HalleyExceptions::VideoPlugin);
    }

    if (FAILED(swapChain->ResizeBuffers(
            numFrames,
            swapChainRenderSize.x,
            swapChainRenderSize.y,
            desc.BufferDesc.Format,
            desc.Flags))) {
        throw Exception("Failed to resize swap chain", HalleyExceptions::VideoPlugin);
    }

    currentFrameIndex = swapChain->GetCurrentBackBufferIndex();

    updateRenderTargetViews();
#endif
}

void DX12Video::updateRenderTargetViews()
{
#ifdef _GAMING_XBOX
    /*
     * No DXGI swap chain, create render-target textures and views.
     */
    {
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask = 1;
        props.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = swapChainRenderSize.x;
        bufferDesc.Height = swapChainRenderSize.y;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = bufferDesc.Format;
        optimizedClearValue.Color[0] = 0.0f;
        optimizedClearValue.Color[1] = 0.0f;
        optimizedClearValue.Color[2] = 0.0f;
        optimizedClearValue.Color[3] = 1.0f;

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = bufferDesc.Format;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        for (uint32_t i = 0; i < numFrames; i++) {
            auto& frame = frames[i];

            if (FAILED(device->CreateCommittedResource(
                    &props,
                    D3D12_HEAP_FLAG_ALLOW_DISPLAY,
                    &bufferDesc,
                    D3D12_RESOURCE_STATE_PRESENT,
                    &optimizedClearValue,
                    D3D12_IID_ARGS(frame.backBuffer)))) {
                throw Exception("Failed to create back buffer", HalleyExceptions::VideoPlugin);
            }

            if (frame.backBufferDescriptor == InvalidDescriptorHandle) {
                frame.backBufferDescriptor = rtvPool->alloc(1);
            }

            auto rtvHandle = rtvPool->getCPUHandle(frame.backBufferDescriptor);
            device->CreateRenderTargetView(frames[i].backBuffer.Get(), &rtvDesc, rtvHandle);
        }
    }
#else
    /*
     * For each frame in the swap chain, obtain the back buffer and create its view.
     */
    for (uint32_t i = 0; i < numFrames; i++) {
        auto& frame = frames[i];

        if (FAILED(swapChain->GetBuffer(i, D3D12_IID_ARGS(frame.backBuffer)))) {
            throw Exception("Failed to obtain swap chain back buffer", HalleyExceptions::VideoPlugin);
        }

        if (frame.backBufferDescriptor == InvalidDescriptorHandle) {
            frame.backBufferDescriptor = rtvPool->alloc(1);
        }

        auto rtvHandle = rtvPool->getCPUHandle(frame.backBufferDescriptor);
        device->CreateRenderTargetView(frames[i].backBuffer.Get(), nullptr, rtvHandle);
    }
#endif

    /*
     * Create depth buffer texture and its depth-stencil view.
     */
    {
        D3D12_HEAP_PROPERTIES props = {};
        props.Type = D3D12_HEAP_TYPE_DEFAULT;
        props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        props.CreationNodeMask = 1;
        props.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = swapChainRenderSize.x;
        bufferDesc.Height = swapChainRenderSize.y;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        if (FAILED(device->CreateCommittedResource(
                &props,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &clearValue,
                D3D12_IID_ARGS(depthBuffer)))) {
            throw Exception("Failed to create depth buffer", HalleyExceptions::VideoPlugin);
        }

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        if (depthBufferDescriptor == InvalidDescriptorHandle) {
            depthBufferDescriptor = dsvPool->alloc(1);
        }

        auto dsvHandle = dsvPool->getCPUHandle(depthBufferDescriptor);
        device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHandle);
    }
}

void DX12Video::waitForGpu()
{
    auto frame = &frames[currentFrameIndex];

    /*
     * Schedule signal command in GPU queue.
     */
    uint64_t fenceValue = frame->fenceValue;

	{
		std::unique_lock lock(commandQueueLock);

    	if (FAILED(commandQueue->Signal(fence.Get(), fenceValue))) {
	        return;
	    }
    }

    /*
     * Wait until the signal has been processed.
     */
    if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent))) {
        WaitForSingleObjectEx(fenceEvent, INFINITE, false);
    }

    /*
     * Increment fence value for the current frame.
     */
    frame->fenceValue++;
}

void DX12Video::moveToNextFrame()
{
    auto frame = &frames[currentFrameIndex];

    /*
     * Schedule signal command.
     */
    uint64_t currentFenceValue = frame->fenceValue;

	{
		std::unique_lock lock(commandQueueLock);

	    if (FAILED(commandQueue->Signal(fence.Get(), currentFenceValue))) {
            throw Exception("Signal() failed", HalleyExceptions::VideoPlugin);
	    }
	}

    /*
     * Update to next frame index.
     */
#ifdef _GAMING_XBOX
    currentFrameIndex = (currentFrameIndex + 1) % numFrames;
#else
    currentFrameIndex = swapChain->GetCurrentBackBufferIndex();
#endif
    frame = &frames[currentFrameIndex];

    /*
     * Wait until the next frame is ready to be rendered.
     */
    if (fence->GetCompletedValue() < frame->fenceValue) {
        if (SUCCEEDED(fence->SetEventOnCompletion(frame->fenceValue, fenceEvent))) {
            WaitForSingleObjectEx(fenceEvent, INFINITE, false);
        }
    }

    /*
     * Set fence value for the next frame.
     */
    frame->fenceValue = currentFenceValue + 1;
}

void DX12Video::registerFrameEvents()
{
#ifdef _GAMING_XBOX
    ComPtr<IDXGIDevice1> dxgiDevice;
    device.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());

    ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf());

    device->SetFrameIntervalX(
            dxgiOutput.Get(),
            D3D12XBOX_FRAME_INTERVAL_60_HZ,
            numFrames,
            D3D12XBOX_FRAME_INTERVAL_FLAG_NONE
    );

    device->ScheduleFrameEventX(
            D3D12XBOX_FRAME_EVENT_ORIGIN,
            0U,
            nullptr,
            D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE
    );
#endif
}

void DX12Video::addRecreateTexture(DX12Texture* texture)
{
    deferRecreateTextures.emplace_back(texture);
}

void DX12Video::addReleaseResource(ComPtr<ID3D12Resource>& resource)
{
    deferReleaseResources.emplace_back(std::make_pair(numFrames + 1, resource));
}

void DX12Video::doDeferredResourceUpdates(bool immediate)
{
    /*
     * Trigger recreation of texture resources (most likely, render targets).
     */
    if (!deferRecreateTextures.empty()) {
        waitForGpu();
        for (auto texture : deferRecreateTextures) {
            texture->doCreateDeferred();
        }
        deferRecreateTextures.clear();
        incrementResourceVersionIndex();
    }

    /*
     * Release resource buffers of deleted DX12Textures.
     *
     * TODO: Invalidates the texture view cache, which does hurt performance.
     */
    size_t count = 0;
    const int step = immediate ? 1000 : 1;

    for (auto& it: deferReleaseResources) {
   		it.first -= step;
        if (it.first <= 0) {
            it.second.Reset();
            count++;
        }
    }

    deferReleaseResources.erase(
            std::remove_if(
                    deferReleaseResources.begin(),
                    deferReleaseResources.end(),
                    [&](const std::pair<int, ComPtr<ID3D12Resource>>& it) -> bool {
                        return it.first <= 0;
                    }),
            deferReleaseResources.end());

    if (count > 0) {
        //Logger::logDev(toString(count) + " resources have been released, invalidating cache!");
        waitForGpu();
        incrementResourceVersionIndex();
    }

    /*
     * This is a safety measure against running dry of descriptors.
     */
    constexpr size_t maxUsagePercent = 80;
    if (srvPool->getUsagePercent() > maxUsagePercent || samPool->getUsagePercent() > maxUsagePercent) {
        Logger::logWarning("Detected high usage of shader resource view / sampler descriptor pools, invalidating cache!");
        incrementResourceVersionIndex();
    }
}

void DX12Video::incrementResourceVersionIndex()
{
    /*
     * Increment the "resource version index". Other places can check on
     * this to trigger eviction of their own resources.
     */
    resourceVersionIndex++;
}
