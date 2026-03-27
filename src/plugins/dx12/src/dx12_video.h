#pragma once

#include "halley_dx12.h"
#include "halley/api/halley_api_internal.h"
#include "halley/graphics/window.h"

#ifdef WITH_GDK
#include <appnotify.h>
#endif

#define MAX_FRAME_BUFFERS 2

namespace Halley {

    class DX12Buffer;
    class DX12DescriptorPool;
    class DX12Loader;
    class DX12Texture;

    class DX12Video final : public VideoAPIInternal
    {
    public:
        explicit DX12Video(SystemAPI& system);
        ~DX12Video() override;

        void init() override;
        void deInit() override;

        static bool isInitialized();
        bool isSuspended() const { return suspended; }

        void onSuspend() override;
        void onResume() override;

        std::unique_ptr<Painter> makePainter(Resources& resources) override;

        void startRender() override;
        void finishRender() override;

        void setWindow(WindowDefinition&& windowDescriptor) override;
        Window& getWindow() const override;
        bool hasWindow() const override;

        void setVsync(bool vsync) override;
        bool hasVsync() const override;
        void waitForVsync() override;
        void flush() override;

        std::unique_ptr<Texture> createTexture(Vector2i size) override;
        std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
        std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
        std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
        std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
        std::unique_ptr<MaterialStructuredBuffer> createStructuredBuffer() override;

        void setScreenRenderTarget();
        D3D12_CPU_DESCRIPTOR_HANDLE getScreenRenderTargetView() const;
        D3D12_CPU_DESCRIPTOR_HANDLE getScreenDepthStencilView() const;

        String getShaderLanguage() override;
        bool isColumnMajor() const override { /* TODO */return false; }

        void* getImplementationPointer(const String& id) override;

        ID3D12Device& getDevice() const { return *device.Get(); }
        ID3D12GraphicsCommandList* getCmdList() const { return commandList.Get(); }

        DX12DescriptorPool* getSrvDescriptorPool() const { return srvPool.get(); }
        DX12DescriptorPool* getSamDescriptorPool() const { return samPool.get(); }
        DX12DescriptorPool* getRtvDescriptorPool() const { return rtvPool.get(); }
        DX12DescriptorPool* getDsvDescriptorPool() const { return dsvPool.get(); }

        DX12Buffer& getConstantBuffer() const { return *constantBuffers[currentFrameIndex]; }
        DX12Buffer& getLoaderTransientBuffer() const { return *loaderTransientBuffer; }

        ID3D12GraphicsCommandList* getLoaderCmdList() const { return loaderCommandList.Get(); }
        ID3D12GraphicsCommandList* startUpload();
        void finishUpload();

        void addRecreateTexture(DX12Texture* texture);
        void addReleaseResource(ComPtr<ID3D12Resource>& resource);

        uint64_t getResourceVersionIndex() const { return resourceVersionIndex; }

        size_t getNumFrames() const { return numFrames; }

    private:
        SystemAPI& system;
        std::shared_ptr<Window> window;

#ifdef _GAMING_XBOX
        D3D12XBOX_FRAME_PIPELINE_TOKEN framePipelineToken = 0u;
#else
        ComPtr<IDXGIFactory6> dxgiFactory;
#if D3D12_DEBUG
        ComPtr<IDXGIDebug> dxgiDebug;
#endif
        ComPtr<IDXGIAdapter4> adapter;
        DXGI_ADAPTER_DESC1 adapterDesc = {};
        ComPtr<IDXGISwapChain4> swapChain;
#endif

        Vector2i swapChainRenderSize;

        ComPtr<ID3D12Device2> device;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<ID3D12GraphicsCommandList> commandList;

        Mutex commandQueueLock;

        std::unique_ptr<DX12DescriptorPool> srvPool;
        std::unique_ptr<DX12DescriptorPool> samPool;
        std::unique_ptr<DX12DescriptorPool> rtvPool;
        std::unique_ptr<DX12DescriptorPool> dsvPool;

        struct Frame
        {
            ComPtr<ID3D12Resource> backBuffer;
            DX12DescriptorHandle backBufferDescriptor = InvalidDescriptorHandle;
            ComPtr<ID3D12CommandAllocator> commandAllocator;
            uint64_t fenceValue = 0;
        } frames[MAX_FRAME_BUFFERS];

        ComPtr<ID3D12Resource> depthBuffer;
        DX12DescriptorHandle depthBufferDescriptor = InvalidDescriptorHandle;

        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent = nullptr;

        uint32_t numFrames = 2;
        uint32_t currentFrameIndex = 0;

        bool useVSync = false;
        bool allowTearing = false;

        std::unique_ptr<DX12Buffer> constantBuffers[MAX_FRAME_BUFFERS];

        std::unique_ptr<DX12Loader> loader;
        std::unique_ptr<DX12Buffer> loaderTransientBuffer;
        ComPtr<ID3D12GraphicsCommandList> loaderCommandList;
        ComPtr<ID3D12CommandAllocator> loaderCommandAllocator;
        ComPtr<ID3D12Fence> loaderFence;
        HANDLE loaderFenceEvent = nullptr;
        uint64_t loaderFenceValue = 0;

        Vector<DX12Texture*> deferRecreateTextures;
        Vector<std::pair<int, ComPtr<ID3D12Resource>>> deferReleaseResources;

        uint64_t resourceVersionIndex = 0;

        static bool initialized;

    	std::atomic<bool> suspended = false;

#ifdef WITH_GDK
        PAPPSTATE_REGISTRATION plmHandle = {};
#endif

        void initDeviceAndSwapChain();
        void shutdownDeviceAndSwapChain();

        void createCommandQueue(ComPtr<ID3D12CommandQueue>& commandQueue, D3D12_COMMAND_LIST_TYPE type);
        void createCommandAllocator(ComPtr<ID3D12CommandAllocator>& allocator, D3D12_COMMAND_LIST_TYPE type);
        void createCommandList(ComPtr<ID3D12GraphicsCommandList>& list, const ComPtr<ID3D12CommandAllocator>& allocator, D3D12_COMMAND_LIST_TYPE type);

        void resizeSwapChain(const Vector2i& windowSize);
        void updateRenderTargetViews();

        void waitForGpu();
        void moveToNextFrame();
        void registerFrameEvents();
        void doDeferredResourceUpdates(bool immediate);
        void incrementResourceVersionIndex();
    };

}
