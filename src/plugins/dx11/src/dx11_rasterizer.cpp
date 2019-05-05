#include "dx11_rasterizer.h"
#include "dx11_video.h"
using namespace Halley;

bool DX11RasterizerOptions::operator==(const DX11RasterizerOptions& other) const
{
	return scissor == other.scissor && culling == other.culling;
}

bool DX11RasterizerOptions::operator!=(const DX11RasterizerOptions& other) const
{
	return !(*this == other);
}

bool DX11RasterizerOptions::operator<(const DX11RasterizerOptions& other) const
{
	if (scissor != other.scissor) {
		return scissor < other.scissor;
	}
	return culling < other.culling;
}

DX11Rasterizer::DX11Rasterizer(DX11Video& video, DX11RasterizerOptions opt)
	: options(opt)
{
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.FillMode = D3D11_FILL_SOLID;
	
	switch (options.culling) {
	case CullingMode::None:
		desc.CullMode = D3D11_CULL_NONE;
		break;
	case CullingMode::Front:
		desc.CullMode = D3D11_CULL_FRONT;
		break;
	case CullingMode::Back:
		desc.CullMode = D3D11_CULL_BACK;
		break;
	}
	
	desc.FrontCounterClockwise = TRUE;
	desc.DepthBias = 0;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthBiasClamp = 0.0f;
	desc.DepthClipEnable = FALSE;
	desc.ScissorEnable = options.scissor;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	HRESULT result = video.getDevice().CreateRasterizerState(&desc, &rasterizer);
	if (result != S_OK) {
		throw Exception("Unable to create rasterizer", HalleyExceptions::VideoPlugin);
	}
}

DX11Rasterizer::~DX11Rasterizer()
{
	if (rasterizer) {
		rasterizer->Release();
		rasterizer = nullptr;
	}
}

void DX11Rasterizer::bind(DX11Video& video)
{
	video.getDeviceContext().RSSetState(rasterizer);
}
