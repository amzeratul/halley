#pragma once
#include "halley/core/graphics/material/material.h"
#include "metal_buffer.h"

namespace Halley
{
  class MetalMaterialConstantBuffer : public MaterialConstantBuffer
  {
  public:
    explicit MetalMaterialConstantBuffer(MetalVideo& video);
    ~MetalMaterialConstantBuffer();
    void update(const MaterialDataBlock& dataBlock) override;
    void bind(id<MTLRenderCommandEncoder> encoder, int bindPoint);
  private:
    MetalBuffer buffer;
  };
}
