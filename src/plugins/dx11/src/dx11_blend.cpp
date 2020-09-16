#include "dx11_blend.h"
#include <gsl/gsl>
#include "dx11_video.h"
#include "halley/core/graphics/blend.h"
using namespace Halley;

DX11Blend::DX11Blend(DX11Video& video, BlendType blend)
{
	D3D11_BLEND_DESC desc;

	desc.AlphaToCoverageEnable = false;
	desc.IndependentBlendEnable = false;

	auto& target = desc.RenderTarget[0];
	target.BlendEnable = true;
	target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	switch (blend) {
	case BlendType::Alpha:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::AlphaPremultiplied:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_ONE;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::Add:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_ONE;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::AddPremultiplied:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_ONE;
		target.DestBlend = D3D11_BLEND_ONE;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::Multiply:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_DEST_COLOR;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::Invert:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendType::Opaque:
	default:
		target.BlendEnable = false;
		break;
	}

	HRESULT result = video.getDevice().CreateBlendState(&desc, &state);
	if (result != S_OK) {
		throw Exception("Unable to create blend state", HalleyExceptions::VideoPlugin);
	}
}

DX11Blend::DX11Blend(DX11Blend&& other) noexcept
	: state(other.state)
{
	other.state = nullptr;
}

DX11Blend::~DX11Blend()
{
	if (state) {
		state->Release();
		state = nullptr;
	}
}

void DX11Blend::bind(DX11Video& video)
{
	Expects(state);
	video.getDeviceContext().OMSetBlendState(state, nullptr, 0xFFFFFFFF);
}

DX11Blend& DX11Blend::operator=(DX11Blend&& other) noexcept
{
	state = other.state;
	other.state = nullptr;
	return *this;
}
