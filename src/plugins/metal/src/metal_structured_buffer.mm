#include "metal_structured_buffer.h"
using namespace Halley;

MetalStructuredBuffer::MetalStructuredBuffer(MetalVideo& video)
	: buffer(video, MetalBuffer::Type::Constant)
{
}

MetalStructuredBuffer::~MetalStructuredBuffer()
{
}

void MetalStructuredBuffer::update(gsl::span<const std::byte> data, size_t elementStride)
{
	buffer.setData(data);
	dataSize = data.size_bytes();
	stride = elementStride;
}

void MetalStructuredBuffer::bindVertex(id<MTLRenderCommandEncoder> encoder, int bindPoint)
{
	buffer.bindVertex(encoder, bindPoint);
}

void MetalStructuredBuffer::bindFragment(id<MTLRenderCommandEncoder> encoder, int bindPoint)
{
	buffer.bindFragment(encoder, bindPoint);
}
