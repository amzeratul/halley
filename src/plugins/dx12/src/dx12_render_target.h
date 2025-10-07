#pragma once

#include "halley_dx12.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/render_target/render_target_texture.h"

namespace Halley {

    class DX12Video;
    class Painter;

    class DX12ScreenRenderTarget final : public ScreenRenderTarget
    {
    public:
        explicit DX12ScreenRenderTarget(DX12Video& video, const Rect4i& viewPort);

        bool getProjectionFlipVertical() const override;
        bool getViewportFlipVertical() const override;

        void onBind(Painter& painter) override;

    private:
        DX12Video& video;
    };

    class DX12TextureRenderTarget final : public TextureRenderTarget
    {
    public:
        explicit DX12TextureRenderTarget(DX12Video& video);
        ~DX12TextureRenderTarget() override;

        bool getProjectionFlipVertical() const override;
        bool getViewportFlipVertical() const override;

        void onBind(Painter& painter) override;
        void update();

        D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetView() const;
        D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilView() const;

        bool enforcePowerOfTwoSize() const override { return true; }

    private:
        DX12Video& video;
        DX12DescriptorHandle rtvHandles = InvalidDescriptorHandle;
        DX12DescriptorHandle dsvHandle = InvalidDescriptorHandle;
        uint64_t curHash = 0;

        void clear();
        void createViews();
        uint64_t getTextureHash() const;
    };

}
