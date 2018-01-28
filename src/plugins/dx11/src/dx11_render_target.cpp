#include "dx11_render_target.h"
using namespace Halley;

DX11ScreenRenderTarget::DX11ScreenRenderTarget(const Rect4i& viewPort)
	: ScreenRenderTarget(viewPort)
{
	
}

bool DX11ScreenRenderTarget::flipVertical() const
{
	return true;
}

void DX11ScreenRenderTarget::onStartDrawCall(Painter&)
{
}

DX11TextureRenderTarget::DX11TextureRenderTarget()
{
	
}
