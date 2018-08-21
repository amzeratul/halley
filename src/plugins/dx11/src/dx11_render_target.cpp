#include "dx11_render_target.h"
#include "dx11_video.h"
#include "dx11_texture.h"
using namespace Halley;

DX11ScreenRenderTarget::DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort, ID3D11RenderTargetView* view)
	: ScreenRenderTarget(viewPort)
	, video(video)
	, view(view)
{
}

bool DX11ScreenRenderTarget::getProjectionFlipVertical() const
{
	return true;
}

bool DX11ScreenRenderTarget::getViewportFlipVertical() const
{
	return false;
}

void DX11ScreenRenderTarget::onBind(Painter& painter)
{
	ID3D11RenderTargetView* views[] = { view };
	video.getDeviceContext().OMSetRenderTargets(1, views, nullptr);
}

ID3D11RenderTargetView* DX11ScreenRenderTarget::getRenderTargetView()
{
	return view;
}

DX11TextureRenderTarget::DX11TextureRenderTarget(DX11Video& video)
	: video(video)
{
}

DX11TextureRenderTarget::~DX11TextureRenderTarget()
{
	clear();
}

bool DX11TextureRenderTarget::getProjectionFlipVertical() const
{
	return true;
}

bool DX11TextureRenderTarget::getViewportFlipVertical() const
{
	return false;
}

void DX11TextureRenderTarget::onBind(Painter& painter)
{
	update();
	video.getDeviceContext().OMSetRenderTargets(UINT(views.size()), views.data(), depthStencilView);
}

ID3D11RenderTargetView* DX11TextureRenderTarget::getRenderTargetView()
{
	update();
	return views.at(0);
}

void DX11TextureRenderTarget::update()
{
	if (dirty) {
		clear();
		createViews();
		dirty = false;
	}
}

void DX11TextureRenderTarget::clear()
{
	for (auto& view: views) {
		view->Release();
	}
	views.clear();

	if (depthStencilView) {
		depthStencilView->Release();
		depthStencilView = nullptr;
	}
}

void DX11TextureRenderTarget::createViews()
{
	views.resize(attachments.size());
	for (size_t i = 0; i < attachments.size(); ++i) {
		auto& texture = static_cast<DX11Texture&>(*attachments[i]);
		texture.waitForLoad();

		D3D11_RENDER_TARGET_VIEW_DESC desc;
		desc.Format = texture.getFormat();
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		auto result = video.getDevice().CreateRenderTargetView(texture.getTexture(), &desc, &views[i]);
		if (result != S_OK) {
			throw Exception("Unable to create render target view for texture attachment #" + toString(i), HalleyExceptions::VideoPlugin);
		}
	}

	if (depth) {
		auto& depthTexture = static_cast<DX11Texture&>(*depth);
		depthTexture.waitForLoad();

		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		desc.Format = depthTexture.getFormat();
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Flags = 0;
		desc.Texture2D.MipSlice = 0;

		auto result = video.getDevice().CreateDepthStencilView(depthTexture.getTexture(), &desc, &depthStencilView);
		if (result != S_OK) {
			throw Exception("Unable to create render target view for depth texture.", HalleyExceptions::VideoPlugin);
		}
	}
}
