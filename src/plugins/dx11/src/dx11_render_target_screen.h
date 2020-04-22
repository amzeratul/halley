#pragma once
#include "dx11_render_target.h"
#include "halley/core/graphics/render_target/render_target_screen.h"

namespace Halley
{
	class DX11SwapChain;

	class DX11ScreenRenderTarget final : public ScreenRenderTarget, public IDX11RenderTarget
	{
	public:
		explicit DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort);

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;
		
		void onBind(Painter& painter) override;

		ID3D11RenderTargetView* getRenderTargetView() override;
		ID3D11DepthStencilView* getDepthStencilView() override;

	private:
		DX11Video& video;
	};
}
