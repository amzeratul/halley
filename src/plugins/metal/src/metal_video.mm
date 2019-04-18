#include "metal_video.h"
#include "metal_material_constant_buffer.h"
#include "metal_render_target.h"

#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/texture_descriptor.h>
#include <halley/core/graphics/shader.h>
#include <halley/core/graphics/material/material_definition.h>
#include <SDL2/SDL.h>

using namespace Halley;

///////////////
// Constructor
MetalVideo::MetalVideo(SystemAPI& system)
  : system(system)
{
}

void MetalVideo::init()
{
  loader = std::make_unique<MetalLoader>(system);
}

void MetalVideo::deInit()
{
  std::cout << "Shutting down Metal..." << std::endl;
  loader.reset();
}

void MetalVideo::startRender()
{
  surface = [swap_chain nextDrawable];
}

void MetalVideo::finishRender()
{
  window->swap();
  [surface release];
}


void MetalVideo::setWindow(WindowDefinition&& windowDescriptor)
{
  window = system.createWindow(windowDescriptor);
  initSwapChain(*window);
}

void MetalVideo::initSwapChain(Window& window) {
  if (window.getNativeHandleType() != "SDL") {
    throw Exception("Only SDL2 windows are supported by Metal", HalleyExceptions::VideoPlugin);
  }
  SDL_Window* sdl_window = static_cast<SDL_Window*>(window.getNativeHandle());
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
  SDL_Renderer *renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
  swap_chain = static_cast<CAMetalLayer*>(SDL_RenderGetMetalLayer(renderer));
  SDL_DestroyRenderer(renderer);
  swap_chain.pixelFormat = MTLPixelFormatBGRA8Unorm;
  device = swap_chain.device;
  command_queue = [device newCommandQueue];
  std::cout << "\tGot Metal device: " << [device.name UTF8String] << std::endl;
}

const Window& MetalVideo::getWindow() const
{
  return *window;
}

bool MetalVideo::hasWindow() const
{
  return window != nullptr;
}


std::unique_ptr<Texture> MetalVideo::createTexture(Vector2i size)
{
  return std::make_unique<MetalTexture>(*this, size);
}

std::unique_ptr<Shader> MetalVideo::createShader(const ShaderDefinition& definition)
{
  return std::make_unique<MetalShader>(*this, definition);
}

std::unique_ptr<TextureRenderTarget> MetalVideo::createTextureRenderTarget()
{
  return std::make_unique<TextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> MetalVideo::createScreenRenderTarget()
{
  return std::make_unique<MetalScreenRenderTarget>(Rect4i({}, getWindow().getWindowRect().getSize()));
}

std::unique_ptr<MaterialConstantBuffer> MetalVideo::createConstantBuffer()
{
  return std::make_unique<MetalMaterialConstantBuffer>(*this);
}

String MetalVideo::getShaderLanguage()
{
  return "metal";
}

std::unique_ptr<Painter> MetalVideo::makePainter(Resources& resources)
{
  return std::make_unique<MetalPainter>(*this, resources);
}

id<CAMetalDrawable> MetalVideo::getSurface() {
  return surface;
}

id<MTLCommandQueue> MetalVideo::getCommandQueue() {
  return command_queue;
}

id<MTLDevice> MetalVideo::getDevice() {
  return device;
}

MetalTexture::MetalTexture(MetalVideo& video, Vector2i size)
  : Texture(size)
  , video(video)
{
  startLoading();
}

void MetalTexture::load(TextureDescriptor&& descriptor)
{
  auto textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
    width:descriptor.size.x
    height:descriptor.size.y
    mipmapped:descriptor.useMipMap
  ];
  metalTexture = [video.getDevice() newTextureWithDescriptor:textureDescriptor];

  NSUInteger bytesPerRow = 4 * descriptor.size.x;
  MTLRegion region = {
    { 0, 0, 0 },
    {static_cast<NSUInteger>(descriptor.size.x), static_cast<NSUInteger>(descriptor.size.y), 1}
  };

  [metalTexture replaceRegion:region
    mipmapLevel:0
    withBytes:descriptor.pixelData.getBytes()
    bytesPerRow:bytesPerRow
  ];

  MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
  samplerDescriptor.maxAnisotropy = 1;
  auto filter = descriptor.useFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
  samplerDescriptor.minFilter = filter;
  samplerDescriptor.magFilter = filter;
  samplerDescriptor.mipFilter = descriptor.useFiltering ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
  auto clamp = descriptor.clamp ? MTLSamplerAddressModeClampToEdge : MTLSamplerAddressModeRepeat;
  samplerDescriptor.sAddressMode = clamp;
  samplerDescriptor.rAddressMode = clamp;
  samplerDescriptor.tAddressMode = clamp;
  samplerDescriptor.lodMinClamp = 0;
  samplerDescriptor.lodMaxClamp = FLT_MAX;
  sampler = [video.getDevice() newSamplerStateWithDescriptor:samplerDescriptor];
  [samplerDescriptor release];

  doneLoading();
}

void MetalTexture::bind(id<MTLRenderCommandEncoder> encoder, int bindIndex) const {
  waitForLoad();

  [encoder setFragmentTexture:metalTexture atIndex:bindIndex];
  [encoder setFragmentSamplerState:sampler atIndex:bindIndex];
}

MetalPainter::MetalPainter(MetalVideo& video, Resources& resources)
  : Painter(resources)
  , video(video)
{}

void MetalPainter::clear(Colour colour) {}

void MetalPainter::setMaterialPass(const Material& material, int passNumber) {
  auto& pass = material.getDefinition().getPass(passNumber);
  // TODO blending
  MetalShader& shader = static_cast<MetalShader&>(pass.getShader());

  MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineStateDescriptor.vertexFunction = shader.getVertexFunc();
  pipelineStateDescriptor.fragmentFunction = shader.getFragmentFunc();
  pipelineStateDescriptor.label = [NSString stringWithUTF8String:material.getDefinition().getName().c_str()];
  pipelineStateDescriptor.colorAttachments[0].pixelFormat = video.getSurface().texture.pixelFormat;
  setBlending(pass.getBlend(), pipelineStateDescriptor.colorAttachments[0]);

  NSError* error = NULL;
  id<MTLRenderPipelineState> pipelineState = [video.getDevice() newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
      error:&error
  ];
  if (!pipelineState) {
    std::cout << "Failed to create pipeline descriptor for material " << material.getDefinition().getName() <<
      ", pass " << passNumber << "." << std::endl;
    throw Exception([[error localizedDescription] UTF8String], HalleyExceptions::VideoPlugin);
  }

  [encoder setRenderPipelineState:pipelineState];

  // Metal requires the global material to be bound for each material pass, as it has no 'global' state.
  static_cast<MetalMaterialConstantBuffer&>(halleyGlobalMaterial->getDataBlocks().front().getConstantBuffer()).bind(encoder, 0);

  // Bind textures
  int texIndex = 0;
  for (auto& tex : material.getTextures()) {
    auto texture = std::static_pointer_cast<const MetalTexture>(tex);
    texture->bind(encoder, texIndex++);
  }
}

void MetalPainter::doStartRender() {
  buffer = [video.getCommandQueue() commandBuffer];
  auto col = Colour4f(0);
  auto descriptor = renderPassDescriptorForTextureAndColour(video.getSurface().texture, col);
  encoder = [buffer renderCommandEncoderWithDescriptor:descriptor];
  [descriptor release];
}

void MetalPainter::doEndRender() {
  [encoder endEncoding];
  [buffer presentDrawable:video.getSurface()];
  [buffer commit];
  [encoder release];
  [buffer release];
  [indexBuffer release];
}

void MetalPainter::setVertices(
  const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices,
  unsigned short* indices, bool standardQuadsOnly
) {
  Expects(numVertices > 0);
  Expects(numIndices >= numVertices);
  Expects(vertexData);
  Expects(indices);

  size_t bytesSize = numVertices * material.getVertexStride();
  id<MTLBuffer> buffer = [video.getDevice() newBufferWithBytes:vertexData
    length:bytesSize
    options:MTLResourceStorageModeShared
  ];
  [encoder setVertexBuffer:buffer offset:0 atIndex:0];

  indexBuffer = [video.getDevice() newBufferWithBytes:indices
      length:numIndices*sizeof(short) options:MTLResourceStorageModeShared
  ];
}

void MetalPainter::drawTriangles(size_t numIndices) {
  Expects(numIndices > 0);
  Expects(numIndices % 3 == 0);

  [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
    indexCount:numIndices
    indexType:MTLIndexTypeUInt16
    indexBuffer:indexBuffer
    indexBufferOffset:0
  ];
}

void MetalPainter::setViewPort(Rect4i rect) {
  [encoder setViewport:(MTLViewport){
    static_cast<double>(rect.getTopLeft().x),
    static_cast<double>(rect.getTopLeft().y),
    static_cast<double>(rect.getWidth()),
    static_cast<double>(rect.getHeight()),
    0.0, 1.0
  }];
}

void MetalPainter::setClip(Rect4i, bool) {}

void MetalPainter::setMaterialData(const Material& material) {
  for (auto& dataBlock : material.getDataBlocks()) {
    if (dataBlock.getType() != MaterialDataBlockType::SharedExternal) {
      static_cast<MetalMaterialConstantBuffer&>(dataBlock.getConstantBuffer()).bind(encoder, dataBlock.getBindPoint());
    }
  }
}

void MetalPainter::onUpdateProjection(Material& material) {
  material.uploadData(*this);
  setMaterialData(material);
}

void MetalPainter::setBlending(BlendType blendType, MTLRenderPipelineColorAttachmentDescriptor* colorAttachment) {
  Expects(
    blendType == BlendType::Alpha || blendType == BlendType::AlphaPremultiplied || blendType == BlendType::Add ||
    blendType == BlendType::Opaque || blendType == BlendType::Multiply || blendType == BlendType::Darken
  );
  bool useBlending = blendType != BlendType::Opaque;
  colorAttachment.blendingEnabled = useBlending;
  if (!useBlending) {
    return;
  }

  colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
  colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;

  switch (blendType) {
    case BlendType::Alpha:
      setBlendFactor(colorAttachment, MTLBlendFactorSourceAlpha, MTLBlendFactorOneMinusSourceAlpha);
      break;
    case BlendType::AlphaPremultiplied:
      setBlendFactor(colorAttachment, MTLBlendFactorOne, MTLBlendFactorOneMinusSourceAlpha);
      break;
    case BlendType::Multiply:
      setBlendFactor(colorAttachment, MTLBlendFactorDestinationColor, MTLBlendFactorOneMinusSourceAlpha);
      break;
    default:
      setBlendFactor(colorAttachment, MTLBlendFactorSourceAlpha, MTLBlendFactorOne);
  }
}

void MetalPainter::setBlendFactor(MTLRenderPipelineColorAttachmentDescriptor* colorAttachment, MTLBlendFactor src, MTLBlendFactor dst) {
  colorAttachment.sourceRGBBlendFactor = src;
  colorAttachment.sourceAlphaBlendFactor = src;
  colorAttachment.destinationRGBBlendFactor = dst;
  colorAttachment.destinationAlphaBlendFactor = dst;
}
