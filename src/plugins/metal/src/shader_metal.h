#pragma once
#include "halley/core/graphics/shader.h"
#include <Metal/Metal.h>

namespace Halley {

  class VideoMetal;

  class MetalShader final : public Shader
  {
  public:
    explicit MetalShader(VideoMetal& video, const ShaderDefinition& definition);
    ~MetalShader();
    int getUniformLocation(const String& name, ShaderType stage) override;
    int getBlockLocation(const String& name, ShaderType stage) override;
    id<MTLFunction> getVertexFunc();
    id<MTLFunction> getFragmentFunc();

  private:
    VideoMetal& video;
    id<MTLFunction> vertex_func;
    id<MTLFunction> fragment_func;
  };
}
