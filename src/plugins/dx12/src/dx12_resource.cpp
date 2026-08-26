#include "dx12_resource.h"

#include "dx12_descriptor_pool.h"
#include "dx12_video.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/utils/hash.h"

using namespace Halley;

#if defined(_GAMING_XBOX)
static constexpr D3D12_RESOURCE_STATES initialDestResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
#else
static constexpr D3D12_RESOURCE_STATES initialDestResourceState = D3D12_RESOURCE_STATE_COMMON;
#endif

static DXGI_FORMAT toFormat(TextureFormat format)
{
    switch (format) {
        case TextureFormat::Indexed:
        case TextureFormat::Red:
            return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::RGB:
            throw Exception("RGB textures are not supported", HalleyExceptions::VideoPlugin);
        case TextureFormat::RGBA:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::Depth:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::BGR565:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case TextureFormat::BGRA5551:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case TextureFormat::BGRX:
            return DXGI_FORMAT_B8G8R8X8_UNORM;
        case TextureFormat::SRGBA:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::RGBAFloat16:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        default:
            break;
    }

    throw Exception("Unknown texture format", HalleyExceptions::VideoPlugin);
}

static D3D12_TEXTURE_ADDRESS_MODE toAddressMode(TextureAddressMode mode)
{
    switch (mode) {
        case TextureAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case TextureAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case TextureAddressMode::Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case TextureAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
    throw Exception("Unknown TextureAddressMode: " + toString(mode), HalleyExceptions::VideoPlugin);
}


DX12Loader::DX12Loader(SystemAPI& system)
    : executor(Executors::getVideoAux())
{
    thread = system.createThread("DX12 Loader", ThreadPriority::Normal, [this]() {
        executor.runForever();
    });
}

DX12Loader::~DX12Loader()
{
    executor.stop();
    thread.join();
}

DX12Buffer::DX12Buffer(DX12Video& video, Type type, size_t initialSize)
    : video(video)
    , type(type)
{
    if (initialSize > 0) {
        resize(initialSize);
    }
}

DX12Buffer::~DX12Buffer()
{
    clear();
}

gsl::span<std::byte> DX12Buffer::map()
{
    const D3D12_RANGE range = {0, 0};
    void* ptr = nullptr;

    if (FAILED(resource->Map(0, &range, &ptr))) {
        throw Exception("Failed to map resource", HalleyExceptions::VideoPlugin);
    }

    return {(std::byte*) ptr, curSize};
}

void DX12Buffer::unmap(size_t written_pos, size_t written_len)
{
    const D3D12_RANGE range = {written_pos, written_pos + written_len};
    resource->Unmap(0, &range);
}

size_t DX12Buffer::copyRows(
        std::byte* dst,
        size_t dst_size,
        size_t dst_row_stride,
        std::byte* src,
        size_t src_row_stride,
        size_t width,
        size_t height,
        size_t bytes_per_element
)
{
    std::byte* d = dst;
    std::byte* s = src;

    size_t row_size = width * bytes_per_element;
    size_t total_size = row_size * height;

    HalleyAssertDev(row_size <= dst_row_stride);
    HalleyAssertDev(height * dst_row_stride <= dst_size);

    while (total_size >= row_size) {
        memcpy(d, s, row_size);
        d += dst_row_stride;
        s += src_row_stride;
        total_size -= row_size;
    }

    return size_t(d - dst);
}

bool DX12Buffer::canFit(size_t size) const
{
	return curWritePos + alignUp(size, size_t(256)) <= curSize;
}

void DX12Buffer::resize(size_t requestedSize)
{
    if (requestedSize <= curSize && curSize != 0) {
        return;
    }

    clear();

    D3D12_HEAP_TYPE heapType;
    D3D12_RESOURCE_STATES initialState = initialDestResourceState;

    switch (type) {
        case Type::DynamicVertex:
        case Type::DynamicIndex: {
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        }

        case Type::Constant:
        case Type::Upload: {
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        }
    }

    D3D12_HEAP_PROPERTIES props = {};
    props.Type = heapType;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask = 1;
    props.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = requestedSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (FAILED(video.getDevice().CreateCommittedResource(
            &props,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            initialState,
            nullptr,
            D3D12_IID_ARGS(resource)))) {
        throw Exception("Failed to create committed resource", HalleyExceptions::VideoPlugin);
    }

    curSize = requestedSize;
}

void DX12Buffer::resetData()
{
    curWritePos = 0;
}

std::pair<size_t, size_t> DX12Buffer::writeData(gsl::span<const std::byte> data)
{
    size_t size = data.size();
    std::pair<size_t, size_t> written = {0, 0};

    size_t align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    /*if (type == Type::Constant)*/ {
        HalleyAssertDev((curWritePos % align) == 0);

        auto dest = map();
        if ((curWritePos + size) > dest.size()) {
            Logger::logError("Buffer resource write overflow. This will cause render errors.");
            return written;
        }

        gsl::copy(data, dest.subspan(curWritePos, size));

        unmap(curWritePos, size);

        written.first = curWritePos;
        written.second = size;

        curWritePos += alignUp(size, align);
    } /*else {
        throw Exception("Not implemented for buffer type", HalleyExceptions::VideoPlugin);
    }*/

    return written;
}

void DX12Buffer::clear()
{
    resource.Reset();
    curSize = 0;
}

DX12Texture::DX12Texture(DX12Video& video, Vector2i size)
    : Texture(size)
    , video(video)
{
}

DX12Texture::~DX12Texture()
{
    clearTexture();
}

DX12Texture& DX12Texture::operator=(DX12Texture&& other) noexcept
{
    other.waitForLoad(true);

    clearTexture();
    moveFrom(other);

    resource = std::move(other.resource);
    resourceDesc = other.resourceDesc;
    state = other.state;
    useFiltering = other.useFiltering;
    addressMode = other.addressMode;
    vramUsage = other.vramUsage;

    other.resourceDesc = {};
    other.state = D3D12_RESOURCE_STATE_COMMON;
    other.vramUsage = 0;

    doneLoading();

    return *this;
}

void DX12Texture::reload(Resource&& resource)
{
    *this = std::move(dynamic_cast<DX12Texture&>(resource));
}

void DX12Texture::doLoad(TextureDescriptor& descriptor)
{
    if (resource == nullptr) {
        doCreateResource(descriptor, false);
    } else {
        video.addRecreateTexture(this);
    }
}

void DX12Texture::clearTexture()
{
    video.removeRecreateTexture(this);
    
    if (resource) {
        video.addReleaseResource(resource);
        resource.Reset();
    }
    resourceDesc = {};
    state = D3D12_RESOURCE_STATE_COMMON;
    vramUsage = 0;
}

void DX12Texture::doCreateDeferred()
{
    bool canKeepResource = canKeepExistingResource(descriptor);

    if (!canKeepResource) {
	    video.addReleaseResource(resource);
    }

    doCreateResource(descriptor, canKeepResource);
}

void DX12Texture::doCreateResource(TextureDescriptor& descriptor, bool keepResource)
{
	size_t bpp = TextureDescriptor::getBytesPerPixel(descriptor.format);

	HalleyAssertDev(!descriptor.useMipMap); // TODO: D3D12 doesn't feature mipmap auto-generation

	if (keepResource) {
        /*
         * Need to transfer back to COPY_DEST if we want to upload pixel data.
         */
	    if (!descriptor.pixelData.empty()) {
		    ID3D12GraphicsCommandList* commandList = video.startUpload();
	        doStateTransition(commandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    } else {
	    useFiltering = descriptor.useFiltering;
	    addressMode = descriptor.addressMode;

	    /*
	     * Create GPU buffer resource in committed memory.
	     */
	    D3D12_HEAP_PROPERTIES props = {};
	    props.Type = D3D12_HEAP_TYPE_DEFAULT;
	    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	    props.CreationNodeMask = 1;
	    props.VisibleNodeMask = 1;

	    D3D12_RESOURCE_DESC desc = {};
	    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	    desc.Alignment = 0;
	    desc.Width = descriptor.size.x;
	    desc.Height = descriptor.size.y;
	    desc.DepthOrArraySize = 1;
	    desc.MipLevels = 1;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
	    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	    desc.Format = toFormat(descriptor.format);

	    state = initialDestResourceState;

	    if (descriptor.canBeReadOnCPU) {
	        desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	    }

	    D3D12_CLEAR_VALUE optimizedClearValue = {};
	    D3D12_CLEAR_VALUE* clearValue = nullptr;

	    if (descriptor.isDepthStencil) {
	        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	        optimizedClearValue.Format = desc.Format;
	        clearValue = &optimizedClearValue;
	        state = D3D12_RESOURCE_STATE_COMMON;
	    } else if (descriptor.isRenderTarget) {
	        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	        optimizedClearValue.Format = desc.Format;
	        optimizedClearValue.DepthStencil.Depth = 1.0f;
	        optimizedClearValue.DepthStencil.Stencil = 0;
	        clearValue = &optimizedClearValue;
	        state = D3D12_RESOURCE_STATE_COMMON;
	    }

	    if (FAILED(video.getDevice().CreateCommittedResource(
	            &props,
	            D3D12_HEAP_FLAG_NONE,
	            &desc,
	            state,
	            clearValue,
	            D3D12_IID_ARGS(resource)))) {
	        throw Exception("Failed to create texture resource", HalleyExceptions::VideoPlugin);
	    }

        resourceDesc = desc;
    }

#ifdef DEV_BUILD
    String name = "Texture:" + getAssetId();
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT) name.size(), name.c_str());
#endif

    vramUsage = bpp * size.x * size.y;

    if (!descriptor.pixelData.empty()) {
        /*
         * Might have started upload above already.
         */
	    ID3D12GraphicsCommandList* commandList = keepResource ? video.getLoaderCmdList() : video.startUpload();

    	/*
         * Map transient buffer and copy pixel data.
         */
        DX12Buffer& transient = video.getLoaderTransientBuffer();
        auto bytes = transient.map();

#ifdef _GAMING_XBOX
        size_t rowPitchAlign = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;
#else
        size_t rowPitchAlign = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
#endif

        HalleyAssertDev(size_t(bytes.data()) % rowPitchAlign == 0);
        size_t rowPitch = alignUp(resourceDesc.Width * bpp, rowPitchAlign);

        size_t bytes_written = DX12Buffer::copyRows(
                bytes.data(),
                bytes.size(),
                rowPitch,
                (std::byte*) descriptor.pixelData.getBytes(),
                resourceDesc.Width * bpp,
                resourceDesc.Width,
                resourceDesc.Height,
                bpp);

        transient.unmap(0, bytes_written);
        HalleyAssertDev(bytes_written == rowPitch * resourceDesc.Height);

        /*
         * Now issue upload command.
         */
        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = resource.Get();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        D3D12_SUBRESOURCE_FOOTPRINT srcFootprint = {};
        srcFootprint.Format = resourceDesc.Format;
        srcFootprint.Width = (uint32_t) resourceDesc.Width;
        srcFootprint.Height = (uint32_t) resourceDesc.Height;
        srcFootprint.Depth = 1;
        srcFootprint.RowPitch = (uint32_t) rowPitch;

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = transient.getResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Offset = 0;
        srcLocation.PlacedFootprint.Footprint = srcFootprint;

        commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

        // D3D12 auto state transition
        state = D3D12_RESOURCE_STATE_COPY_DEST;

        doStateTransition(commandList,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

        video.finishUpload();
	}

    doneLoading();
}

bool DX12Texture::canKeepExistingResource(const TextureDescriptor& descriptor) const
{
    // TODO: this doesn't check descriptor flags yet
	return descriptor.size.x == int(resourceDesc.Width) &&
        descriptor.size.y == int(resourceDesc.Height) &&
        toFormat(descriptor.format) == resourceDesc.Format &&
        descriptor.useFiltering == useFiltering &&
        descriptor.addressMode == addressMode;
}

void DX12Texture::doStateTransition(ID3D12GraphicsCommandList* commandList,
                                    D3D12_RESOURCE_STATES targetState,
                                    D3D12_RESOURCE_STATES fromState) const
{
    if (targetState != state) {
        if (fromState == state || state == D3D12_RESOURCE_STATE_COMMON) {
            D3D12_RESOURCE_BARRIER barrierDesc = {};
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Transition.pResource = resource.Get();
            barrierDesc.Transition.StateBefore = state;
            barrierDesc.Transition.StateAfter = targetState;
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            commandList->ResourceBarrier(1, &barrierDesc);

            state = targetState;
        }
    }
}

DXGI_FORMAT DX12Texture::getFormat() const
{
    return toFormat(this->getDescriptor().format);
}

D3D12_TEXTURE_ADDRESS_MODE DX12Texture::getAddressMode() const
{
    return toAddressMode(addressMode);
}

D3D12_FILTER DX12Texture::getFilter() const
{
    return useFiltering ? D3D12_FILTER_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_POINT;
}

void DX12Texture::hash(Hash::Hasher& hasher) const
{
    waitForLoad();
    hasher.feed(resource.Get());
    hasher.feedBytes(gsl::as_bytes(gsl::span<const TextureDescriptor>(&descriptor, 1)));
}

size_t DX12Texture::getVRamUsage() const
{
	return vramUsage;
}

void DX12Texture::doCopyToTexture(Painter& painter, Texture& other) const
{
    auto& otherDX = dynamic_cast<DX12Texture&>(other);
    if (!resource || !otherDX.resource) {
        Logger::logError("DX12Texture doCopyToTexture failed due to resource or other.resource being invalid!");
        return;
    }

    ID3D12GraphicsCommandList* commandList = video.getCmdList();
    const auto srcOldState = state;
    const auto dstOldState = otherDX.state;

    doStateTransition(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, srcOldState);
    otherDX.doStateTransition(commandList, D3D12_RESOURCE_STATE_COPY_DEST, dstOldState);

    commandList->CopyResource(otherDX.resource.Get(), resource.Get());

    doStateTransition(commandList, srcOldState, D3D12_RESOURCE_STATE_COPY_SOURCE);
    otherDX.doStateTransition(commandList, dstOldState, D3D12_RESOURCE_STATE_COPY_DEST);
}

void DX12Texture::doCopyToImage(Painter& painter, Image& image) const
{
    if (!resource) {
        Logger::logError("DX12Texture doCopyToImage failed due to resource being invalid!");
        return;
    }

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    UINT64 totalBytes = 0;

    video.getDevice().GetCopyableFootprints(&resourceDesc, 0, 1, 0, &layout, &numRows, &rowSizeInBytes, &totalBytes);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = totalBytes;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> readbackBuffer;
    if (FAILED(video.getDevice().CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, D3D12_IID_ARGS(readbackBuffer)))) {
        throw Exception("DX12Texture doCopyToImage Failed to create readback buffer", HalleyExceptions::VideoPlugin);
    }

    ID3D12GraphicsCommandList* commandList = video.getCmdList();
    const auto oldState = state;
    doStateTransition(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, oldState);

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = layout;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = resource.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    doStateTransition(commandList, oldState, D3D12_RESOURCE_STATE_COPY_SOURCE);

    video.flushMainCommandList();

    D3D12_RANGE readRange = { 0, totalBytes };
    void* mappedData = nullptr;
    if (FAILED(readbackBuffer->Map(0, &readRange, &mappedData))) {
        throw Exception("DX12Texture doCopyToImage failed to map readbackBuffer", HalleyExceptions::VideoPlugin);
    }

    const size_t h = std::min(static_cast<size_t>(image.getSize().y), static_cast<size_t>(resourceDesc.Height));
    const size_t w = std::min(static_cast<size_t>(image.getSize().x), static_cast<size_t>(resourceDesc.Width));
    const char* src = static_cast<const char*>(mappedData) + layout.Offset;
    const auto dst = image.getPixels4BPP();

    if (descriptor.format == TextureFormat::RGBA || descriptor.format == TextureFormat::SRGBA) {
        for (size_t y = 0; y < h; ++y) {
            const auto* srcPx = reinterpret_cast<const uint32_t*>(src + y * layout.Footprint.RowPitch);
            const auto dstPx = dst.subspan(y * w, w);
            for (size_t x = 0; x < w; ++x) {
                dstPx[x] = srcPx[x];
            }
        }
    }
    else if (descriptor.format == TextureFormat::BGR565) {
        for (size_t y = 0; y < h; ++y) {
            const auto* srcPx = reinterpret_cast<const uint16_t*>(src + y * layout.Footprint.RowPitch);
            const auto dstPx = dst.subspan(y * w, w);
            for (size_t x = 0; x < w; ++x) {
                const auto px = static_cast<uint32_t>(srcPx[x]);
                const auto b = (px & 0x001F) << 19;
                const auto g = (px & 0x07E0) << 5;
                const auto r = (px & 0xF800) >> 8;
                const auto a = static_cast<uint32_t>(0xFF) << 24;
                dstPx[x] = r | g | b | a;
            }
        }
    }
    else if (descriptor.format == TextureFormat::BGRX) {
        for (size_t y = 0; y < h; ++y) {
            const auto* srcPx = reinterpret_cast<const uint32_t*>(src + y * layout.Footprint.RowPitch);
            const auto dstPx = dst.subspan(y * w, w);
            for (size_t x = 0; x < w; ++x) {
                uint32_t px = srcPx[x];
                const uint32_t ag = px & 0xFF00FF00u;
                const uint32_t r = (px & 0x00FF0000u) >> 16;
                const uint32_t b = (px & 0x000000FFu) << 16;
                dstPx[x] = r | b | ag;
            }
        }
    }
    else {
        D3D12_RANGE emptyRange = { 0, 0 };
        readbackBuffer->Unmap(0, &emptyRange);
        throw Exception("DX12Texture doCopyToImage unable to copy texture format to image: " + toString(descriptor.format), HalleyExceptions::VideoPlugin);
    }

    D3D12_RANGE emptyRange = { 0, 0 };
    readbackBuffer->Unmap(0, &emptyRange);
}

DX12TextureView::DX12TextureView(DX12Video& video)
    : video(video)
{
}

DX12TextureView::~DX12TextureView()
{
    clear();
}

void DX12TextureView::bind(const Material& material)
{
    uint64_t hash = getTextureHash(material);

    if (hash != curHash) {
        clear();
        createViews(material);
        curHash = hash;
    }

    auto srvPool = video.getSrvDescriptorPool();
    auto samPool = video.getSamDescriptorPool();

    auto cmdList = video.getCmdList();

    for (size_t i = 0; i < material.getNumTextureUnits(); i++) {
        auto texture = getNthTexture(material, i);
        texture->doStateTransition(cmdList,
                                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    auto rootParameterIndex = (uint32_t) material.getDefinition().getUniformBlocks().size();

    auto srvGpuHandle = srvPool->getGPUHandle(srvHandles);
    cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvGpuHandle);

    auto samGpuHandle = samPool->getGPUHandle(samplerHandles);
    cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex + 1, samGpuHandle);
}

void DX12TextureView::clear()
{
    /*
     * This happens in well-defined places, and after a wait-for-gpu,
     * so it should be safe to just free those handles.
     */
    if (srvHandles != InvalidDescriptorHandle) {
        video.getSrvDescriptorPool()->free(srvHandles);
        srvHandles = InvalidDescriptorHandle;
    }

    if (samplerHandles != InvalidDescriptorHandle) {
        video.getSamDescriptorPool()->free(samplerHandles);
        samplerHandles = InvalidDescriptorHandle;
    }

    curHash = 0;
}

void DX12TextureView::createViews(const Material& material)
{
    auto srvPool = video.getSrvDescriptorPool();
    auto samPool = video.getSamDescriptorPool();

    size_t count = material.getNumTextureUnits();

    srvHandles = srvPool->alloc(count);
    samplerHandles = samPool->alloc(count);

    for (size_t i = 0; i < count; i++) {
        auto texture = getNthTexture(material, i);

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = toFormat(texture->getDescriptor().format);
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        desc.Texture2D.MipLevels = 1;

        video.getDevice().CreateShaderResourceView(
                texture->getResource(),
                &desc,
                srvPool->getCPUHandle(srvHandles, i));

        D3D12_TEXTURE_ADDRESS_MODE addressMode = texture->getAddressMode();

        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = texture->getFilter();
        samplerDesc.AddressU = addressMode;
        samplerDesc.AddressV = addressMode;
        samplerDesc.AddressW = addressMode;
        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor[0] = 1.0f;
        samplerDesc.BorderColor[1] = 1.0f;
        samplerDesc.BorderColor[2] = 1.0f;
        samplerDesc.BorderColor[3] = 1.0f;
        samplerDesc.MinLOD = -FLT_MAX;
        samplerDesc.MaxLOD = FLT_MAX;

        video.getDevice().CreateSampler(
                &samplerDesc,
                samPool->getCPUHandle(samplerHandles, i));
    }
}

uint64_t DX12TextureView::getTextureHash(const Material& material)
{
    Hash::Hasher hasher;

    for (size_t i = 0; i < material.getNumTextureUnits(); i++) {
        auto texture = getNthTexture(material, i);
        texture->hash(hasher);
    }

    return hasher.digest();
}

DX12Texture* DX12TextureView::getNthTexture(const Material& material, size_t n)
{
    HalleyAssertDev(material.getTextures().size() > n);

    auto& texture = material.getTexture(int(n));
    HalleyAssertDev(texture.get() != nullptr);

    return (DX12Texture*) texture.get();
}

DX12MaterialConstantBuffer::DX12MaterialConstantBuffer(DX12Video& video)
    : video(video)
    , buffer(video, DX12Buffer::Type::Constant, 0)
{

}

void DX12MaterialConstantBuffer::update(gsl::span<const std::byte> data)
{
    size_t size = data.size_bytes();
    buffer.resize(size);

    auto dest = buffer.map();
    gsl::copy(data, dest);
    buffer.unmap(0, size);
}
