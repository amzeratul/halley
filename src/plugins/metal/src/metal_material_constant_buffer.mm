#include "metal_material_constant_buffer.h"
#include "halley/utils/utils.h"
#include "metal_video.h"
#include <gsl/gsl>

using namespace Halley;

MetalMaterialConstantBuffer::MetalMaterialConstantBuffer(MetalVideo& video)
	: buffer(video, MetalBuffer::Type::Constant)
{}

MetalMaterialConstantBuffer::~MetalMaterialConstantBuffer() {}

void MetalMaterialConstantBuffer::update(gsl::span<const gsl::byte> data) {
  // We must pad up to a multiple of 16 (float4)
  // TODO we ought to move this somewhere it won't be called so often.
  const size_t padded_size = alignUp<size_t>(data.size_bytes(), 16);

  auto padded = malloc(padded_size);
  memcpy(padded, data.data(), data.size_bytes());

  buffer.setData(gsl::span{reinterpret_cast<gsl::byte *>(padded),
                           static_cast<std::size_t>(static_cast<long>(padded_size))});

  free(padded);
}


void MetalMaterialConstantBuffer::bindVertex(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	buffer.bindVertex(encoder, bindPoint);
}

void MetalMaterialConstantBuffer::bindFragment(id<MTLRenderCommandEncoder> encoder, int bindPoint) {
	buffer.bindFragment(encoder, bindPoint);
}
