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
		virtual ~IDX11RenderTarget() {}

		virtual ID3D11RenderTargetView* getRenderTargetView() = 0;
		virtual ID3D11DepthStencilView* getDepthStencilView() = 0;
	};

	class DX11ScreenRenderTarget : public ScreenRenderTarget, public IDX11RenderTarget
	{
	public:
		explicit DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort, ID3D11RenderTargetView* renderTargetView, ID3D11DepthStencilView* depthStencilView);

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;
		
		void onBind(Painter& painter) override;

		ID3D11RenderTargetView* getRenderTargetView() override;
		ID3D11DepthStencilView* getDepthStencilView() override;

	private:
		DX11Video& video;
		ID3D11RenderTargetView* renderTargetView;
		ID3D11DepthStencilView* depthStencilView;
	};

	class DX11TextureRenderTarget : public TextureRenderTarget, public IDX11RenderTarget
	{
	public:
		DX11TextureRenderTarget(DX11Video& video);
		~DX11TextureRenderTarget();

		bool getProjectionFlipVertical() const override;
		bool getViewportFlipVertical() const override;
		
		void onBind(Painter& painter) override;

		ID3D11RenderTargetView* getRenderTargetView() override;
		ID3D11DepthStencilView* getDepthStencilView() override;

	private:
		DX11Video& video;
		Vector<ID3D11RenderTargetView*> views;
		ID3D11DepthStencilView* depthStencilView = nullptr;

		void update();
		void clear();
		void createViews();
	};
}
