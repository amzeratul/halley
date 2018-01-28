#include "dx11_painter.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "dx11_shader.h"
#include "dx11_material_constant_buffer.h"
#include "dx11_blend.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "dx11_texture.h"
using namespace Halley;

DX11Painter::DX11Painter(DX11Video& video, Resources& resources)
	: Painter(resources)
	, video(video)
	, vertexBuffer(video, DX11Buffer::Type::Vertex)
	, indexBuffer(video, DX11Buffer::Type::Index)
{
}

void DX11Painter::doStartRender()
{
	if (!rasterizer) {
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.FrontCounterClockwise = TRUE;
		desc.DepthBias = 0;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthBiasClamp = 0.0f;
		desc.DepthClipEnable = FALSE;
		desc.ScissorEnable = FALSE;
		desc.MultisampleEnable = FALSE;
		desc.AntialiasedLineEnable = FALSE;

		video.getDevice().CreateRasterizerState(&desc, &rasterizer);
		video.getDeviceContext().RSSetState(rasterizer);
	}
}

void DX11Painter::doEndRender()
{
	if (rasterizer) {
		rasterizer->Release();
		rasterizer = nullptr;
	}
}

void DX11Painter::clear(Colour colour)
{
	const float col[] = { colour.r, colour.g, colour.b, colour.a };
	video.getDeviceContext().ClearRenderTargetView(&video.getRenderTarget(), col);
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
			throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.");					
		} else {
			ID3D11ShaderResourceView* srvs[] = { texture->getShaderResourceView() };
			video.getDeviceContext().PSSetShaderResources(textureUnit, 1, srvs);
		}
		++textureUnit;
	}
}

void DX11Painter::setMaterialData(const Material& material)
{
	auto& devCon = video.getDeviceContext();
	for (auto& block: material.getDataBlocks()) {
		if (block.getType() != MaterialDataBlockType::SharedExternal) {
			auto buffer = static_cast<DX11MaterialConstantBuffer&>(block.getConstantBuffer()).getBuffer().getBuffer();
			devCon.VSSetConstantBuffers(block.getBindPoint(), 1, &buffer);
			devCon.PSSetConstantBuffers(block.getBindPoint(), 1, &buffer);
		}
	}
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
	auto& devCon = video.getDeviceContext();
	devCon.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devCon.DrawIndexed(UINT(numIndices), 0, 0);
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
