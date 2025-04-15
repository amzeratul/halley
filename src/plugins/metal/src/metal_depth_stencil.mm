#include "metal_depth_stencil.h"
#include "metal_video.h"
using namespace Halley;

static MTLCompareFunction getComparisonFunc(DepthStencilComparisonFunction f)
{
	switch (f) {
	case DepthStencilComparisonFunction::Always:
		return MTLCompareFunctionAlways;
	case DepthStencilComparisonFunction::Never:
		return MTLCompareFunctionNever;
	case DepthStencilComparisonFunction::Equal:
		return MTLCompareFunctionEqual;
	case DepthStencilComparisonFunction::NotEqual:
		return MTLCompareFunctionNotEqual;
	case DepthStencilComparisonFunction::Less:
		return MTLCompareFunctionLess;
	case DepthStencilComparisonFunction::LessEqual:
		return MTLCompareFunctionLessEqual;
	case DepthStencilComparisonFunction::Greater:
		return MTLCompareFunctionGreater;
	case DepthStencilComparisonFunction::GreaterEqual:
		return MTLCompareFunctionGreaterEqual;
	}

	return MTLCompareFunctionNever;
}

static MTLStencilOperation getOperation(StencilWriteOperation op)
{
	switch (op) {
	case StencilWriteOperation::Zero:
		return MTLStencilOperationZero;
	case StencilWriteOperation::Invert:
		return MTLStencilOperationInvert;
	case StencilWriteOperation::Keep:
		return MTLStencilOperationKeep;
	case StencilWriteOperation::Replace:
		return MTLStencilOperationReplace;
	case StencilWriteOperation::IncrementClamp:
		return MTLStencilOperationIncrementClamp;
	case StencilWriteOperation::IncrementWrap:
		return MTLStencilOperationIncrementWrap;
	case StencilWriteOperation::DecrementClamp:
		return MTLStencilOperationDecrementClamp;
	case StencilWriteOperation::DecrementWrap:
		return MTLStencilOperationDecrementWrap;
	}
	return MTLStencilOperationKeep;
}

MetalDepthStencil::MetalDepthStencil(MetalVideo& video, const MaterialDepthStencil& definition)
	: video(video)
	, definition(definition)
{
	MTLDepthStencilDescriptor * desc = [[MTLDepthStencilDescriptor alloc] init];

	desc.depthWriteEnabled = definition.isDepthWriteEnabled();
	desc.depthCompareFunction = getComparisonFunc(definition.isDepthTestEnabled() ? definition.getDepthComparisonFunction() : DepthStencilComparisonFunction::Always);

	if( definition.isStencilTestEnabled() ) {

		MTLStencilDescriptor *stencil_desc = [[MTLStencilDescriptor alloc] init];

		stencil_desc.readMask = definition.getStencilReadMask();
		stencil_desc.writeMask = definition.getStencilWriteMask();

		stencil_desc.stencilFailureOperation = getOperation(definition.getStencilOpStencilFail());
		stencil_desc.depthFailureOperation = getOperation(definition.getStencilOpDepthFail());
		stencil_desc.depthStencilPassOperation = getOperation(definition.getStencilOpPass());
		stencil_desc.stencilCompareFunction = getComparisonFunc(definition.getStencilComparisonFunction());

		desc.frontFaceStencil = stencil_desc;
		desc.backFaceStencil = stencil_desc;

		reference = definition.getStencilReference();
	}

	state = [video.getDevice() newDepthStencilStateWithDescriptor:desc];
	if (state == nil) {
		throw Exception("Unable to create DepthStencil state", HalleyExceptions::VideoPlugin);
	}
}

MetalDepthStencil::~MetalDepthStencil()
{
	if (state) {
		[state release];
		state = nullptr;
	}
}

const MaterialDepthStencil& MetalDepthStencil::getDefinition() const
{
	return definition;
}

void MetalDepthStencil::bind( id<MTLRenderCommandEncoder> encoder )
{
	[encoder setDepthStencilState: state];
	if (definition.isStencilTestEnabled())
	{
		[encoder setStencilReferenceValue: reference];
	}
}
