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

	switch (blend.mode) {
	case BlendMode::Alpha:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = blend.premultiplied ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendMode::Add:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = blend.premultiplied ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_ONE;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendMode::Multiply:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_DEST_COLOR;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendMode::Invert:
		target.BlendOp = D3D11_BLEND_OP_ADD;
		target.SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
		target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		break;

	case BlendMode::Max:
		target.BlendOp = D3D11_BLEND_OP_MAX;
		target.SrcBlend = blend.premultiplied ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_ONE;
		target.BlendOpAlpha = D3D11_BLEND_OP_MAX;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_ONE;
		break;

	case BlendMode::Min:
		target.BlendOp = D3D11_BLEND_OP_MIN;
		target.SrcBlend = blend.premultiplied ? D3D11_BLEND_ONE : D3D11_BLEND_SRC_ALPHA;
		target.DestBlend = D3D11_BLEND_ONE;
		target.BlendOpAlpha = D3D11_BLEND_OP_MIN;
		target.SrcBlendAlpha = D3D11_BLEND_ONE;
		target.DestBlendAlpha = D3D11_BLEND_ONE;
		break;

	case BlendMode::Opaque:
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
