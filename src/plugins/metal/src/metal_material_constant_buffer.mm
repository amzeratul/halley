#include "metal_material_constant_buffer.h"
#include "metal_video.h"

using namespace Halley;

MetalMaterialConstantBuffer::MetalMaterialConstantBuffer(MetalVideo& video)
	: buffer(video, MetalBuffer::Type::Constant)
{}

MetalMaterialConstantBuffer::~MetalMaterialConstantBuffer() {}

void MetalMaterialConstantBuffer::update(const MaterialDataBlock& dataBlock) {
	buffer.setData(dataBlock.getData());
}


void MetalMaterialConstantBuffer::bind(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	buffer.bind(encoder, bindPoint);
}
