#pragma once

#include "halley_dx12.h"
#include "halley/graphics/painter.h"
#include "halley/data_structures/time_cache.h"

namespace Halley {

    class DX12Buffer;
    class DX12Pipeline;
    class DX12TextureView;
    class DX12Video;

    class DX12Painter final : public Painter
    {
    public:
        explicit DX12Painter(DX12Video &video, Resources &resources);

    private:
        DX12Video& dx12Video;
        TimeCache<uint64_t, std::unique_ptr<DX12Pipeline>, uint32_t> pipelines;
        TimeCache<uint64_t, std::unique_ptr<DX12TextureView>, uint32_t> textureViews;
        HashMap<uint32_t, std::pair<size_t, size_t>> constantBufferCache;

        Vector<DX12Buffer> vertexBuffers;
        Vector<DX12Buffer> indexBuffers;
        size_t curBuffer = 0;

        Rect4i curViewport;
        std::optional<Rect4i> clipping;

        uint64_t resourceVersionIndex = 0;

        void resetState() override;

        void doStartRender() override;
        void doEndRender() override;

        void rotateBuffers();
        void setVertices(const MaterialDefinition& material, size_t numVertices, const void* vertexData, size_t numIndices, const IndexType* indices, bool standardQuadsOnly) override;
        void drawTriangles(size_t numIndices) override;

        void doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil) override;

        void setMaterialPass(const Material& material, int pass) override;
        void setMaterialData(const Material& material) override;

        void setViewPort(Rect4i rect) override;
        void setClip(Rect4i clip, bool enable) override;

        void onUpdateProjection(Material& material, bool hashChanged) override;

        DX12Pipeline& getPipeline(const Material& material);
        DX12TextureView& getTextureView(const Material& material);

        void doEvictTextureViewCache();
    };

}
