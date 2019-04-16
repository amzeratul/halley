#pragma once
#include "metal_shader.h"
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/graphics/render_target/render_target_screen.h"
#include "halley/core/graphics/painter.h"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

namespace Halley {

  static inline MTLRenderPassDescriptor* renderPassDescriptorForTextureAndColour(id<MTLTexture> texture, Colour& colour) {
    MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].clearColor = MTLClearColorMake(colour.r, colour.g, colour.b, colour.a);
    pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].texture = texture;
    return pass;
  }

  class MetalVideo final : public VideoAPIInternal
  {
  public:
    explicit MetalVideo(SystemAPI& system);

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

    id<CAMetalDrawable> getSurface();
    id<MTLCommandQueue> getCommandQueue();
    id<MTLDevice> getDevice();

  private:
    std::shared_ptr<Window> window;
    SystemAPI& system;
    CAMetalLayer* swap_chain;
    id<CAMetalDrawable> surface;
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;

    void initSwapChain(Window& window);
  };


  class MetalTexture : public Texture
  {
  public:
    explicit MetalTexture(Vector2i size);
    void load(TextureDescriptor&& descriptor) override;
  };

  class MetalMaterialConstantBuffer : public MaterialConstantBuffer
  {
  public:
    void update(const MaterialDataBlock& dataBlock) override;
  };

  class MetalPainter : public Painter
  {
  public:
    explicit MetalPainter(MetalVideo& video, Resources& resources);
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

  private:
    MetalVideo& video;
    id<MTLCommandBuffer> buffer;
    id<MTLRenderCommandEncoder> encoder;
    id<MTLBuffer> indexBuffer;
  };
}
