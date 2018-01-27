#include "dx11_painter.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "dx11_shader.h"
using namespace Halley;

DX11Painter::DX11Painter(DX11Video& video, Resources& resources)
	: Painter(resources)
	, video(video)
	, vertexBuffer(video)
	, indexBuffer(video)
{
}

void DX11Painter::doStartRender()
{
}

void DX11Painter::doEndRender()
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

void DX11Painter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly)
{
	const size_t stride = material.getVertexStride();
	const size_t vertexDataSize = stride * numVertices;
	vertexBuffer.setData(gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(vertexData), vertexDataSize));

	ID3D11Buffer* buffers[] = { vertexBuffer.getBuffer() };
	UINT strides[] = { UINT(stride) };
	UINT offsets[] = { 0 };
	video.getDeviceContext().IASetVertexBuffers(0, 1, buffers, strides, offsets);

	indexBuffer.setData(gsl::as_bytes(gsl::span<unsigned short>(indices, numIndices)));
	video.getDeviceContext().IASetIndexBuffer(indexBuffer.getBuffer(), DXGI_FORMAT_R16_UINT, 0);
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
