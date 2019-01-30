#include "dx11_painter.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "dx11_shader.h"
#include "dx11_material_constant_buffer.h"
#include "dx11_blend.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "dx11_texture.h"
#include "dx11_rasterizer.h"
#include "halley/core/graphics/render_target/render_target.h"
#include "dx11_render_target.h"
#include "halley/core/game/game_platform.h"
using namespace Halley;

DX11Painter::DX11Painter(DX11Video& video, Resources& resources)
	: Painter(resources)
	, video(video)
{
#ifdef WINDOWS_STORE
	// Due to the architecture of "some" platforms available here, updating a dynamic buffer is extremely slow if it's still in use
	constexpr size_t numBuffers = 2;
#else
	constexpr size_t numBuffers = 1;
#endif
	for (size_t i = 0; i < numBuffers; ++i) {
		vertexBuffers.emplace_back(video, DX11Buffer::Type::Vertex, 8 * 1024 * 1024);
		indexBuffers.emplace_back(video, DX11Buffer::Type::Index, 128 * 1024);
	}
}

void DX11Painter::doStartRender()
{
	if (!normalRaster) {
		normalRaster = std::make_unique<DX11Rasterizer>(video, false);
		scissorRaster = std::make_unique<DX11Rasterizer>(video, true);
	}
	normalRaster->bind(video);
}

void DX11Painter::doEndRender()
{
}

void DX11Painter::clear(Colour colour)
{
	const float col[] = { colour.r, colour.g, colour.b, colour.a };
	auto view = dynamic_cast<IDX11RenderTarget&>(getActiveRenderTarget()).getRenderTargetView();
	video.getDeviceContext().ClearRenderTargetView(view, col);
}

void DX11Painter::setMaterialPass(const Material& material, int passN)
{
	auto& pass = material.getDefinition().getPass(passN);

	// Shader
	auto& shader = static_cast<DX11Shader&>(pass.getShader());
	shader.setMaterialLayout(video, material.getDefinition().getAttributes());
	shader.bind(video);

	// Blend
	getBlendMode(pass.getBlend()).bind(video);

	// Texture
	int textureUnit = 0;
	for (auto& tex: material.getTextureUniforms()) {
		auto texture = std::static_pointer_cast<const DX11Texture>(material.getTexture(textureUnit));
		if (!texture) {
			throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.", HalleyExceptions::VideoPlugin);
		} else {
			texture->bind(video, textureUnit);
		}
		++textureUnit;
	}
}

void DX11Painter::setMaterialData(const Material& material)
{
	auto& devCon = video.getDeviceContext();
	for (auto& block: material.getDataBlocks()) {
		if (block.getType() != MaterialDataBlockType::SharedExternal) {
			auto& buffer = static_cast<DX11MaterialConstantBuffer&>(block.getConstantBuffer()).getBuffer();
			auto dxBuffer = buffer.getBuffer();
			if (Halley::getPlatform() == GamePlatform::UWP || Halley::getPlatform() == GamePlatform::XboxOne) {
				UINT firstConstant[] = { buffer.getOffset() / 16 };
				UINT numConstants[] = { buffer.getLastSize() / 16 };
				devCon.VSSetConstantBuffers1(block.getBindPoint(), 1, &dxBuffer, firstConstant, numConstants);
				devCon.PSSetConstantBuffers1(block.getBindPoint(), 1, &dxBuffer, firstConstant, numConstants);
			} else {
				devCon.VSSetConstantBuffers(block.getBindPoint(), 1, &dxBuffer);
				devCon.PSSetConstantBuffers(block.getBindPoint(), 1, &dxBuffer);
			}
		}
	}
}

void DX11Painter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly)
{
	const size_t stride = material.getVertexStride();
	const size_t vertexDataSize = stride * numVertices;

	if (!vertexBuffers[curBuffer].canFit(vertexDataSize) || !indexBuffers[curBuffer].canFit(numIndices * sizeof(unsigned short))) {
		rotateBuffers();
	}

	{
		auto& vb = vertexBuffers[curBuffer];
		vb.setData(gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(vertexData), vertexDataSize));
		ID3D11Buffer* buffers[] = { vb.getBuffer() };
		UINT strides[] = { UINT(stride) };
		UINT offsets[] = { vb.getOffset() };
		video.getDeviceContext().IASetVertexBuffers(0, 1, buffers, strides, offsets);
	}

	{
		auto& ib = indexBuffers[curBuffer];
		ib.setData(gsl::as_bytes(gsl::span<unsigned short>(indices, numIndices)));
		video.getDeviceContext().IASetIndexBuffer(ib.getBuffer(), DXGI_FORMAT_R16_UINT, ib.getOffset());
	}
}

void DX11Painter::drawTriangles(size_t numIndices)
{
	auto& devCon = video.getDeviceContext();
	devCon.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devCon.DrawIndexed(UINT(numIndices), 0, 0);
}

void DX11Painter::setViewPort(Rect4i rect)
{
	auto fRect = Rect4f(rect);

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = fRect.getTopLeft().x;
	viewport.TopLeftY = fRect.getTopLeft().y;
	viewport.Width = fRect.getWidth();
	viewport.Height = fRect.getHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	video.getDeviceContext().RSSetViewports(1, &viewport);
}

void DX11Painter::setClip(Rect4i clip, bool enable)
{
	if (enable) {
		scissorRaster->bind(video);
		D3D11_RECT rect;
		rect.top = clip.getTop();
		rect.bottom = clip.getBottom();
		rect.left = clip.getLeft();
		rect.right = clip.getRight();
		video.getDeviceContext().RSSetScissorRects(1, &rect);
	} else {
		normalRaster->bind(video);
	}
}

void DX11Painter::onUpdateProjection(Material& material)
{
	material.uploadData(*this);
	setMaterialData(material);
}

DX11Blend& DX11Painter::getBlendMode(BlendType type)
{
	auto iter = blendModes.find(type);
	if (iter != blendModes.end()) {
		return iter->second;
	}

	blendModes.emplace(std::make_pair(type, DX11Blend(video, type)));
	return getBlendMode(type);
}

void DX11Painter::rotateBuffers()
{
	Expects (vertexBuffers.size() == indexBuffers.size());
	curBuffer = (curBuffer + 1) % vertexBuffers.size();
	indexBuffers[curBuffer].reset();
	vertexBuffers[curBuffer].reset();
}
