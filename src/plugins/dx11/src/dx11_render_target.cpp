#include "dx11_render_target.h"
#include "dx11_video.h"
using namespace Halley;

DX11ScreenRenderTarget::DX11ScreenRenderTarget(DX11Video& video, const Rect4i& viewPort)
	: ScreenRenderTarget(viewPort)
	, video(video)
{
	
}

bool DX11ScreenRenderTarget::flipVertical() const
{
	return true;
}

void DX11ScreenRenderTarget::onBind(Painter& painter)
{
	//video.getDeviceContext().OMSetRenderTargets(0, nullptr, nullptr);
}

DX11TextureRenderTarget::DX11TextureRenderTarget(DX11Video& video)
	: video(video)
{
}

void DX11TextureRenderTarget::onBind(Painter& painter)
{
	
}
