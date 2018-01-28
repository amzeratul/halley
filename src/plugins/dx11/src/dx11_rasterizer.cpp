#include "dx11_rasterizer.h"
#include "dx11_video.h"
using namespace Halley;

DX11Rasterizer::DX11Rasterizer(DX11Video& video, bool enableScissor)
{
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));

	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;
	desc.FrontCounterClockwise = TRUE;
	desc.DepthBias = 0;
	desc.SlopeScaledDepthBias = 0.0f;
	desc.DepthBiasClamp = 0.0f;
	desc.DepthClipEnable = FALSE;
	desc.ScissorEnable = enableScissor;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	video.getDevice().CreateRasterizerState(&desc, &rasterizer);
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
