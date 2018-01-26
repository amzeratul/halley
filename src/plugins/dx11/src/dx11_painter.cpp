#include "dx11_painter.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "dx11_shader.h"
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

void DX11Painter::setMaterialPass(const Material& material, int passN)
{
	auto& pass = material.getDefinition().getPass(passN);
	auto& shader = static_cast<DX11Shader&>(pass.getShader());
	shader.bind(video);
}

void DX11Painter::setMaterialData(const Material& material)
{
	// TODO
}

void DX11Painter::doStartRender()
{
	// TODO
}

void DX11Painter::doEndRender()
{
	// TODO
}

void DX11Painter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly)
{
	// TODO
}

void DX11Painter::drawTriangles(size_t numIndices)
{
	// TODO
}

void DX11Painter::setViewPort(Rect4i rect)
{
	// TODO
}

void DX11Painter::setClip(Rect4i clip, bool enable)
{
	// TODO
}

void DX11Painter::onUpdateProjection(Material& material)
{
	// TODO
}
