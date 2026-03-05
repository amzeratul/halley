#include "dx12_painter.h"

#include "dx12_pipeline.h"
#include "dx12_render_target.h"
#include "dx12_resource.h"
#include "dx12_video.h"
#include "halley/graphics/material/material_definition.h"

using namespace Halley;

DX12Painter::DX12Painter(DX12Video &video, Resources &resources)
    : Painter(video, resources)
    , dx12Video(video)
{
    const size_t numBuffers = video.getNumFrames() * 2;

    for (size_t i = 0; i < numBuffers; ++i) {
        vertexBuffers.emplace_back(video, DX12Buffer::Type::DynamicVertex, 8 * 1024 * 1024);
        indexBuffers.emplace_back(video, DX12Buffer::Type::DynamicIndex, 128 * 1024);
    }
}

void DX12Painter::resetState()
{
    Painter::resetState();
}

void DX12Painter::doStartRender()
{
    uint64_t currentResourceVersionIndex = dx12Video.getResourceVersionIndex();
    if (currentResourceVersionIndex > resourceVersionIndex) {
        doEvictTextureViewCache();
        resourceVersionIndex = currentResourceVersionIndex;
    }

    rotateBuffers(); // TODO?
}

void DX12Painter::doEndRender()
{

}

void DX12Painter::rotateBuffers()
{
    HalleyAssertDev(vertexBuffers.size() == indexBuffers.size());
    curBuffer = (curBuffer + 1) % vertexBuffers.size();
    indexBuffers[curBuffer].resetData();
    vertexBuffers[curBuffer].resetData();
}

void DX12Painter::setVertices(const MaterialDefinition& material,
                              size_t numVertices, const void* vertexData,
                              size_t numIndices, const IndexType* indices,
                              bool standardQuadsOnly)
{
    const size_t stride = material.getVertexStride();
    const size_t vertexDataSize = stride * numVertices;
    const size_t indexDataSize = numIndices * sizeof(short);

    if (!vertexBuffers[curBuffer].canFit(vertexDataSize) || !indexBuffers[curBuffer].canFit(indexDataSize)) {
	    rotateBuffers();
    }

    auto cmdList = dx12Video.getCmdList();

    {
        auto& vb = vertexBuffers[curBuffer];
        auto range = vb.writeData(gsl::span((std::byte*) vertexData, vertexDataSize));
        HalleyAssertDev(vertexDataSize == range.second);

        D3D12_VERTEX_BUFFER_VIEW view = {};
        view.BufferLocation = vb.getResource()->GetGPUVirtualAddress() + range.first;
        view.SizeInBytes = UINT(vertexDataSize);
        view.StrideInBytes = UINT(stride);

        cmdList->IASetVertexBuffers(0, 1, &view);
    }

    {
        auto& ib = indexBuffers[curBuffer];
        auto range = ib.writeData(gsl::span((std::byte*) indices, indexDataSize));
        HalleyAssertDev(indexDataSize == range.second);

        D3D12_INDEX_BUFFER_VIEW view = {};
        view.BufferLocation = ib.getResource()->GetGPUVirtualAddress() + range.first;
        view.SizeInBytes = UINT(indexDataSize);
        view.Format = DXGI_FORMAT_R16_UINT;

        cmdList->IASetIndexBuffer(&view);
    }
}

void DX12Painter::drawTriangles(size_t numIndices)
{
    auto cmdList = dx12Video.getCmdList();
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawIndexedInstanced(uint32_t(numIndices), 1, 0, 0, 0);
}

void DX12Painter::doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
    flush();

    auto renderTarget = tryGetActiveRenderTarget();

    if (renderTarget == nullptr) {
        return;
    }

    auto renderTargetREAL = dynamic_cast<const DX12TextureRenderTarget*>(renderTarget);

    auto cmdList = dx12Video.getCmdList();

    if (colour) {
        const auto& c = colour.value();
        const float col[] = { c.r, c.g, c.b, c.a };

        D3D12_CPU_DESCRIPTOR_HANDLE rtv;
        if (renderTargetREAL == nullptr) {
            rtv = dx12Video.getScreenRenderTargetView();
        } else {
            rtv = renderTargetREAL->getRenderTargetView();
        }

        cmdList->ClearRenderTargetView(
                rtv,
                col,
                0,
                nullptr
        );
    }

    if (depth || stencil) {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv;
        if (renderTargetREAL == nullptr) {
            dsv = dx12Video.getScreenDepthStencilView();
        } else {
            dsv = renderTargetREAL->getDepthStencilView();
        }

        if (dsv.ptr != 0) {
	        cmdList->ClearDepthStencilView(
	                dsv,
	                D3D12_CLEAR_FLAG_DEPTH,
	                1.0f,
	                0,
	                0,
	                nullptr
	        );
        }
    }
}

void DX12Painter::setMaterialPass(const Material& material, int passN)
{
    auto& pipeline = getPipeline(material);
    pipeline.bind(dx12Video, material, passN);

    auto cmdList = dx12Video.getCmdList();
    auto& constantBuffer = dx12Video.getConstantBuffer();

    for (auto& block : material.getDataBlocks()) {
        int bindPoint = block.getBindPoint();
        HalleyAssertDev(constantBufferCache.contains(bindPoint));
        // Update CBV
        size_t address = constantBuffer.getResource()->GetGPUVirtualAddress();
        size_t offset = constantBufferCache[bindPoint].first;
        cmdList->SetGraphicsRootConstantBufferView(bindPoint, address + offset);
    }

    if (material.getNumTextureUnits() > 0) {
        auto& textureView = getTextureView(material);
        textureView.bind(material);
    }

    auto clip = clipping.value_or(curViewport);

    D3D12_RECT scissor = {
            clip.getLeft(),
            clip.getTop(),
            clip.getRight(),
            clip.getBottom(),
    };

#ifdef _GAMING_XBOX
    /*
     * Debug validation error on Xbox: "Valid Rects have 0 <= x < ScissorMax=0x4000"
     */
    scissor.left = std::clamp<LONG>(scissor.left, 0, 0x3fff);
    scissor.top = std::clamp<LONG>(scissor.top, 0, 0x3fff);
    scissor.right = std::clamp<LONG>(scissor.right, 0, 0x3fff);
    scissor.bottom = std::clamp<LONG>(scissor.bottom, 0, 0x3fff);
#endif

	/*
     * It's possible for callers to pass a zero-sized clip rectangle, but this
     * causes validation errors. [WARNING #695: DRAW_EMPTY_SCISSOR_RECTANGLE]
     */

	if (scissor.right == scissor.left) {
	    scissor.right = scissor.left + 1;
    }
    if (scissor.bottom == scissor.top) {
	    scissor.bottom = scissor.top + 1;
    }

    dx12Video.getCmdList()->RSSetScissorRects(1, &scissor);
}

void DX12Painter::setMaterialData(const Material& material)
{
    auto& constantBuffer = dx12Video.getConstantBuffer();

    for (auto& block : material.getDataBlocks()) {
        if (block.getType() != MaterialDataBlockType::SharedExternal) {
            // Stream into per-frame constant buffer
            auto range = constantBuffer.writeData(block.getData());
            // Cache until we actually need (are able) to bind CBVs.
            if (range.first == range.second) {
                break; // TODO: Overflow. Bad. Maybe crash immediately instead.
            }
            constantBufferCache[block.getBindPoint()] = range;
        }
    }
}

void DX12Painter::setViewPort(Rect4i rect)
{
    curViewport = rect;

    auto fRect = Rect4f(rect);

    const D3D12_VIEWPORT viewport = {
            fRect.getTopLeft().x,
            fRect.getTopLeft().y,
            fRect.getWidth(),
            fRect.getHeight(),
            0.0f,
            1.0f,
    };

    dx12Video.getCmdList()->RSSetViewports(1, &viewport);
}

void DX12Painter::setClip(Rect4i clip, bool enable)
{
    clipping = enable ? clip : std::optional<Rect4i>();
}

void DX12Painter::onUpdateProjection(Material& material, bool hashChanged)
{
    /*if (hashChanged)*/ {
        setMaterialData(material);
    }
}

DX12Pipeline& DX12Painter::getPipeline(const Material& material)
{
    uint64_t hash = DX12Pipeline::getMaterialHash(material);

    return *pipelines.get(hash, [&]() {
        return std::make_unique<DX12Pipeline>(material.getDefinition().getNumPasses());
    });
}

DX12TextureView& DX12Painter::getTextureView(const Material& material)
{
    uint64_t hash = DX12TextureView::getTextureHash(material);

    return *textureViews.get(hash, [&]() {
        return std::make_unique<DX12TextureView>(dx12Video);
    });
}

void DX12Painter::doEvictTextureViewCache()
{
    textureViews.getHashMap().clear();
}
