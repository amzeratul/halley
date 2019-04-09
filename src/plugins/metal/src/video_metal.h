#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/graphics/render_target/render_target_screen.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/painter.h"
#include "Metal/Metal.h"

namespace Halley {
  class VideoMetal final : public VideoAPIInternal
  {
  public:
    explicit VideoMetal(SystemAPI& system);

    void startRender() override;
    void finishRender() override;
    void setWindow(WindowDefinition&& windowDescriptor) override;
    const Window& getWindow() const override;
    bool hasWindow() const override;
    std::unique_ptr<Texture> createTexture(Vector2i size) override;
    std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
    std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
    std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
    std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
    void init() override;
    void deInit() override;
    std::unique_ptr<Painter> makePainter(Resources& resources) override;
    String getShaderLanguage() override;

  private:
    std::shared_ptr<Window> window;
    SystemAPI& system;
    id<MTLDevice> device;
  };


  class MetalTexture : public Texture
  {
  public:
    explicit MetalTexture(Vector2i size);
    void load(TextureDescriptor&& descriptor) override;
  };

  class MetalShader : public Shader
  {
  public:
    int getUniformLocation(const String& name, ShaderType stage) override;
    int getBlockLocation(const String& name, ShaderType stage) override;
  };

  class MetalMaterialConstantBuffer : public MaterialConstantBuffer
  {
  public:
    void update(const MaterialDataBlock& dataBlock) override;
  };

  class MetalPainter : public Painter
  {
  public:
    explicit MetalPainter(Resources& resources);
    void clear(Colour colour) override;
    void setMaterialPass(const Material& material, int pass) override;
    void doStartRender() override;
    void doEndRender() override;
    void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) override;
    void drawTriangles(size_t numIndices) override;
    void setViewPort(Rect4i rect) override;
    void setClip(Rect4i clip, bool enable) override;
    void setMaterialData(const Material& material) override;
    void onUpdateProjection(Material& material) override;
  };
}
