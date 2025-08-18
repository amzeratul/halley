#pragma once

#include "halley_dx12.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material.h"

namespace Halley {

    class DX12Video;
    class SystemAPI;

    class DX12Loader
    {
    public:
        explicit DX12Loader(SystemAPI& system);
        ~DX12Loader();

    private:
        Executor executor;
        std::thread thread;
    };

    class DX12Buffer
    {
    public:
        enum class Type
        {
            DynamicVertex,
            DynamicIndex,
            Constant,
            Upload
        };

        DX12Buffer(DX12Video& video, Type type, size_t initialSize);
        ~DX12Buffer();

        ID3D12Resource* getResource() const { return resource.Get(); }

        gsl::span<std::byte> map();
        void unmap(size_t written_pos, size_t written_len);

        static size_t copyRows(
                std::byte* dst,
                size_t dst_size,
                size_t dst_row_stride,
                std::byte* src,
                size_t src_row_stride,
                size_t width,
                size_t height,
                size_t bytes_per_element
        );

        bool canFit(size_t size) const;
        void resize(size_t requestedSize);

        void resetData();
        std::pair<size_t, size_t> writeData(gsl::span<const std::byte> data);

    private:
        DX12Video& video;
        const Type type;
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        size_t curSize = 0;

        size_t curWritePos = 0;

        void clear();
    };

    class DX12Texture final : public Texture
    {
    public:
        explicit DX12Texture(DX12Video& video, Vector2i size);
        ~DX12Texture() override;

        DX12Texture& operator=(DX12Texture&& other) noexcept;

        void doLoad(TextureDescriptor& descriptor) override;
        void doCreateDeferred();

        void doStateTransition(ID3D12GraphicsCommandList* commandList,
                               D3D12_RESOURCE_STATES targetState,
                               D3D12_RESOURCE_STATES fromState) const;

        ID3D12Resource* getResource() const { return resource.Get(); }
        DXGI_FORMAT getFormat() const;
        D3D12_TEXTURE_ADDRESS_MODE getAddressMode() const;
        D3D12_FILTER getFilter() const;

        void hash(Hash::Hasher& hasher) const;

    private:
        DX12Video& video;
        ComPtr<ID3D12Resource> resource;
        D3D12_RESOURCE_DESC resourceDesc = {};
        mutable D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

        bool useFiltering = false;
        TextureAddressMode addressMode;

        void doCreateResource(TextureDescriptor& descriptor, bool keepResource);
        bool canKeepExistingResource(const TextureDescriptor& descriptor) const;
    };

    class DX12TextureView
    {
    public:
        explicit DX12TextureView(DX12Video& video);
        ~DX12TextureView();

        void bind(const Material& material);

        static uint64_t getTextureHash(const Material& material);

    private:
        DX12Video& video;
        DX12DescriptorHandle srvHandles = InvalidDescriptorHandle;
        DX12DescriptorHandle samplerHandles = InvalidDescriptorHandle;
        uint64_t curHash = 0;

        void clear();
        void createViews(const Material& material);
        static DX12Texture* getNthTexture(const Material& material, size_t n);
    };

    class DX12MaterialConstantBuffer final : public MaterialConstantBuffer
    {
    public:
        explicit DX12MaterialConstantBuffer(DX12Video& video);

        void update(gsl::span<const std::byte> data) override;

        DX12Buffer& getBuffer() { return buffer; }

    private:
        DX12Video& video;
        DX12Buffer buffer;
    };

}
