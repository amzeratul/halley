#include "metal_buffer.h"
#include "metal_video.h"

using namespace Halley;

MetalBuffer::MetalBuffer(MetalVideo& video, Type type, size_t initialSize)
	: video(video)
	, type(type)
{}

MetalBuffer::~MetalBuffer() {
	[buffer setPurgeableState:MTLPurgeableStateEmpty];
	[buffer release];
}

void MetalBuffer::setData(gsl::span<const gsl::byte> data) {
	auto oldBuffer = std::move(buffer);
	buffer = [video.getDevice() newBufferWithBytes:data.data() length:data.length_bytes() options:MTLResourceStorageModeShared];
	[oldBuffer setPurgeableState:MTLPurgeableStateEmpty];
	[oldBuffer release];
}

void MetalBuffer::bind(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	[encoder setVertexBuffer:buffer offset:0 atIndex:bindPoint+1];
	[encoder setFragmentBuffer:buffer offset:0 atIndex:bindPoint+1];
}
