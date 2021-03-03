#include "dx11_render_target_screen.h"
#include "dx11_video.h"
#include "dx11_swapchain.h"
using namespace Halley;

DX11ScreenRenderTarget::DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort)
	: ScreenRenderTarget(viewPort)
	, video(video)
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
	auto& swapChain = video.getSwapChain();
	auto* renderTarget = swapChain.getRenderTargetView();
	video.getDeviceContext().OMSetRenderTargets(1, &renderTarget, swapChain.getDepthStencilView());
}

ID3D11RenderTargetView* DX11ScreenRenderTarget::getRenderTargetView()
{
	return video.getSwapChain().getRenderTargetView();
}

ID3D11DepthStencilView* DX11ScreenRenderTarget::getDepthStencilView()
{
	return video.getSwapChain().getDepthStencilView();
}
