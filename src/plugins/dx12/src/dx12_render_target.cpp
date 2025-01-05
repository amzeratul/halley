#include "dx12_render_target.h"

#include "dx12_descriptor_pool.h"
#include "dx12_resource.h"
#include "dx12_video.h"
#include "halley/utils/hash.h"

using namespace Halley;

DX12ScreenRenderTarget::DX12ScreenRenderTarget(DX12Video& video, const Rect4i& viewPort)
    : ScreenRenderTarget(viewPort)
    , video(video)
{

}

bool DX12ScreenRenderTarget::getProjectionFlipVertical() const
{
    return true;
}

bool DX12ScreenRenderTarget::getViewportFlipVertical() const
{
    return false;
}

void DX12ScreenRenderTarget::onBind(Painter& painter)
{
    video.setScreenRenderTarget();
}

DX12TextureRenderTarget::DX12TextureRenderTarget(DX12Video& video)
    : video(video)
{

}

DX12TextureRenderTarget::~DX12TextureRenderTarget()
{
    clear();
}

bool DX12TextureRenderTarget::getProjectionFlipVertical() const
{
    return true;
}

bool DX12TextureRenderTarget::getViewportFlipVertical() const
{
    return false;
}

void DX12TextureRenderTarget::onBind(Painter& painter)
{
    update();

    auto cmdList = video.getCmdList();

    for (size_t i = 0; i < colourBuffer.size(); i++) {
        auto& texture = (DX12Texture&) *colourBuffer[i];
        texture.doStateTransition(cmdList,
                                  D3D12_RESOURCE_STATE_RENDER_TARGET,
                                  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    if (depthStencilBuffer != nullptr) {
        auto& depthTexture = (DX12Texture&) *depthStencilBuffer;
        depthTexture.doStateTransition(cmdList,
                                       D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                       D3D12_RESOURCE_STATE_COMMON);
    }

    auto rtv = video.getRtvDescriptorPool()->getCPUHandle(rtvHandles);

    if (depthStencilBuffer == nullptr) {
        cmdList->OMSetRenderTargets((UINT) colourBuffer.size(), &rtv, true, nullptr);
    } else {
        auto dsv = video.getDsvDescriptorPool()->getCPUHandle(dsvHandle);
        cmdList->OMSetRenderTargets((UINT) colourBuffer.size(), &rtv, true, &dsv);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureRenderTarget::getRenderTargetView() const
{
    return video.getRtvDescriptorPool()->getCPUHandle(rtvHandles);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12TextureRenderTarget::getDepthStencilView() const
{
    if (dsvHandle != InvalidDescriptorHandle) {
		return video.getDsvDescriptorPool()->getCPUHandle(dsvHandle);
    } else {
	    return {};
    }
}

void DX12TextureRenderTarget::update()
{
    if (dirty) {
        /*
         * Ignore the dirty flag. This is raised (and checked) mid-frame.
         * We wait for recreation of the actual render target textures,
         * which will happen at start of the next frame.
         */
        dirty = false;
    }

    uint64_t hash = getTextureHash();

    if (hash != curHash) {
        clear();
        createViews();
        curHash = hash;
    }
}

void DX12TextureRenderTarget::clear()
{
    if (rtvHandles != InvalidDescriptorHandle) {
        video.getRtvDescriptorPool()->free(rtvHandles);
        rtvHandles = InvalidDescriptorHandle;
    }

    if (dsvHandle != InvalidDescriptorHandle) {
        video.getDsvDescriptorPool()->free(dsvHandle);
        dsvHandle = InvalidDescriptorHandle;
    }
}

void DX12TextureRenderTarget::createViews()
{
    rtvHandles = video.getRtvDescriptorPool()->alloc(colourBuffer.size());

    for (size_t i = 0; i < colourBuffer.size(); i++) {
        auto& texture = (DX12Texture&) *colourBuffer[i];

        D3D12_RENDER_TARGET_VIEW_DESC desc = {};
        desc.Format = texture.getFormat();
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        auto rtv = video.getRtvDescriptorPool()->getCPUHandle(rtvHandles, i);

        video.getDevice().CreateRenderTargetView(texture.getResource(), &desc, rtv);
    }

    if (depthStencilBuffer != nullptr) {
        dsvHandle = video.getDsvDescriptorPool()->alloc(1);

        auto& depthTexture = (DX12Texture&) *depthStencilBuffer;

        D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
        desc.Format = depthTexture.getFormat();
        desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Flags = D3D12_DSV_FLAG_NONE;

        auto dsv = video.getDsvDescriptorPool()->getCPUHandle(dsvHandle);

        video.getDevice().CreateDepthStencilView(depthTexture.getResource(), &desc, dsv);
    }
}

uint64_t DX12TextureRenderTarget::getTextureHash() const
{
    Hash::Hasher hasher;

    for (const auto& texture : colourBuffer) {
        auto t = (const DX12Texture*) texture.get();
        t->hash(hasher);
    }

    if (depthStencilBuffer != nullptr) {
	    auto d = (const DX12Texture*) depthStencilBuffer.get();
	    d->hash(hasher);
    }

    return hasher.digest();
}
