#pragma once
#include "halley/core/graphics/render_target/render_target_screen.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include <d3d11.h>
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class IDX11RenderTarget
	{
	public:
		~IDX11RenderTarget() {}

		virtual ID3D11RenderTargetView* getRenderTargetView() = 0;
	};

	class DX11ScreenRenderTarget : public ScreenRenderTarget, public IDX11RenderTarget
	{
	public:
		explicit DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort, ID3D11RenderTargetView* view);

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;
		
		void onBind(Painter& painter) override;

		ID3D11RenderTargetView* getRenderTargetView() override;

	private:
		DX11Video& video;
		ID3D11RenderTargetView* view;
	};

	class DX11TextureRenderTarget : public TextureRenderTarget, public IDX11RenderTarget
	{
	public:
		DX11TextureRenderTarget(DX11Video& video);
		~DX11TextureRenderTarget();

		void onBind(Painter& painter) override;

		ID3D11RenderTargetView* getRenderTargetView() override;

	private:
		DX11Video& video;
		Vector<ID3D11RenderTargetView*> views;
		ID3D11DepthStencilView* depthStencilView = nullptr;

		void update();
		void clear();
		void createViews();
	};
}
