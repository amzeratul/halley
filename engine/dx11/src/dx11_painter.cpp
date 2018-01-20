#include "dx11_painter.h"
#include "dx11_video.h"
using namespace Halley;

DX11Painter::DX11Painter(DX11Video& video, Resources& resources)
	: Painter(resources)
	, video(video)
{
}

void DX11Painter::clear(Colour colour)
{
	const float col[] = { colour.r, colour.g, colour.b, colour.a };
	video.getDeviceContext().ClearRenderTargetView(&video.getRenderTarget(), col);
}

void DX11Painter::setMaterialPass(const Material& material, int pass) {}

void DX11Painter::setMaterialData(const Material& material) {}

void DX11Painter::doStartRender() {}

void DX11Painter::doEndRender() {}

void DX11Painter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) {}

void DX11Painter::drawTriangles(size_t numIndices) {}

void DX11Painter::setViewPort(Rect4i rect) {}

void DX11Painter::setClip(Rect4i clip, bool enable) {}

void DX11Painter::onUpdateProjection(Material& material) {}
