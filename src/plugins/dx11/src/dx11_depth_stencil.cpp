#include "dx11_depth_stencil.h"
#include "halley/core/graphics/material/material_definition.h"
#include "dx11_video.h"
using namespace Halley;

static D3D11_COMPARISON_FUNC getComparisonFunc(DepthStencilComparisonFunction f)
{
	switch (f) {
	case DepthStencilComparisonFunction::Always:
		return D3D11_COMPARISON_ALWAYS;
	case DepthStencilComparisonFunction::Never:
		return D3D11_COMPARISON_NEVER;
	case DepthStencilComparisonFunction::Equal:
		return D3D11_COMPARISON_EQUAL;
	case DepthStencilComparisonFunction::NotEqual:
		return D3D11_COMPARISON_NOT_EQUAL;
	case DepthStencilComparisonFunction::Less:
		return D3D11_COMPARISON_LESS;
	case DepthStencilComparisonFunction::LessEqual:
		return D3D11_COMPARISON_LESS_EQUAL;
	case DepthStencilComparisonFunction::Greater:
		return D3D11_COMPARISON_GREATER;
	case DepthStencilComparisonFunction::GreaterEqual:
		return D3D11_COMPARISON_GREATER_EQUAL;
	}

	return D3D11_COMPARISON_NEVER;
}

static D3D11_STENCIL_OP getOperation(StencilWriteOperation op)
{
	switch (op) {
	case StencilWriteOperation::Zero:
		return D3D11_STENCIL_OP_ZERO;
	case StencilWriteOperation::Invert:
		return D3D11_STENCIL_OP_INVERT;
	case StencilWriteOperation::Keep:
		return D3D11_STENCIL_OP_KEEP;
	case StencilWriteOperation::Replace:
		return D3D11_STENCIL_OP_REPLACE;
	case StencilWriteOperation::IncrementClamp:
		return D3D11_STENCIL_OP_INCR_SAT;
	case StencilWriteOperation::IncrementWrap:
		return D3D11_STENCIL_OP_INCR;
	case StencilWriteOperation::DecrementClamp:
		return D3D11_STENCIL_OP_DECR_SAT;
	case StencilWriteOperation::DecrementWrap:
		return D3D11_STENCIL_OP_DECR;
	}
	return D3D11_STENCIL_OP_KEEP;
}

DX11DepthStencil::DX11DepthStencil(DX11Video& video, const MaterialDepthStencil& definition)
	: video(video)
{
	D3D11_DEPTH_STENCIL_DESC desc;

	desc.DepthEnable = definition.isDepthTestEnabled() || definition.isDepthWriteEnabled();
	desc.DepthWriteMask = definition.isDepthWriteEnabled() ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = getComparisonFunc(definition.isDepthTestEnabled() ? definition.getDepthComparisonFunction() : DepthStencilComparisonFunction::Always);

	desc.StencilEnable = definition.isStencilTestEnabled();
	desc.StencilReadMask = definition.getStencilReadMask();
	desc.StencilWriteMask = definition.getStencilWriteMask();

	desc.FrontFace.StencilFailOp = getOperation(definition.getStencilOpStencilFail());
	desc.FrontFace.StencilDepthFailOp = getOperation(definition.getStencilOpStencilFail());
	desc.FrontFace.StencilPassOp = getOperation(definition.getStencilOpStencilFail());
	desc.FrontFace.StencilFunc = getComparisonFunc(definition.getStencilComparisonFunction());

	desc.BackFace.StencilFailOp = desc.FrontFace.StencilFailOp;
	desc.BackFace.StencilDepthFailOp = desc.FrontFace.StencilDepthFailOp;
	desc.BackFace.StencilPassOp = desc.FrontFace.StencilPassOp;
	desc.BackFace.StencilFunc = desc.FrontFace.StencilFunc;

	video.getDevice().CreateDepthStencilState(&desc, &state);
}

DX11DepthStencil::~DX11DepthStencil()
{
	if (state) {
		state->Release();
		state = nullptr;
	}
}
