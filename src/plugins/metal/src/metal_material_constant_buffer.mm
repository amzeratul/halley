#include "metal_material_constant_buffer.h"
#include "halley/utils/utils.h"
#include "metal_video.h"
#include <gsl/gsl>

using namespace Halley;

MetalMaterialConstantBuffer::MetalMaterialConstantBuffer(MetalVideo& video)
	: buffer(video, MetalBuffer::Type::Constant)
{}

MetalMaterialConstantBuffer::~MetalMaterialConstantBuffer() {}

void MetalMaterialConstantBuffer::update(const MaterialDataBlock& dataBlock) {
  // We must pad up to a multiple of 16 (float4)
  // TODO we ought to move this somewhere it won't be called so often.
  auto data = dataBlock.getData();
  const size_t padding = alignUp<char>(data.size_bytes(), 16);

  auto padded = malloc(data.size_bytes() + padding);
  memcpy(padded, data.data(), data.size_bytes());

  buffer.setData(gsl::span{reinterpret_cast<gsl::byte *>(padded),
                           static_cast<std::size_t>(static_cast<long>(data.size_bytes() + padding))});

  free(padded);
}


void MetalMaterialConstantBuffer::bindVertex(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	buffer.bindVertex(encoder, bindPoint);
}

void MetalMaterialConstantBuffer::bindFragment(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	buffer.bindFragment(encoder, bindPoint);
}
