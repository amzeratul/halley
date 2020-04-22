#pragma once
#include "dx11_render_target.h"
#include "halley/core/graphics/render_target/render_target_texture.h"

namespace Halley
{
	class DX11TextureRenderTarget final : public TextureRenderTarget, public IDX11RenderTarget
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
