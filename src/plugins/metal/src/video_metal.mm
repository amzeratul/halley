#include "video_metal.h"
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
#include <SDL2/SDL.h>

using namespace Halley;

///////////////
// Constructor
VideoMetal::VideoMetal(SystemAPI& system)
  : system(system)
{
}

void VideoMetal::init()
{
}

void VideoMetal::deInit()
{
  std::cout << "Shutting down Metal..." << std::endl;
}

void VideoMetal::startRender()
{
  surface = [swap_chain nextDrawable];
}

void VideoMetal::finishRender()
{
  window->swap();
  [surface release];
}


void VideoMetal::setWindow(WindowDefinition&& windowDescriptor)
{
  window = system.createWindow(windowDescriptor);
  initSwapChain(*window);
}

void VideoMetal::initSwapChain(Window& window) {
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

const Window& VideoMetal::getWindow() const
{
  return *window;
}

bool VideoMetal::hasWindow() const
{
  return window != nullptr;
}


std::unique_ptr<Texture> VideoMetal::createTexture(Vector2i size)
{
  return std::make_unique<MetalTexture>(size);
}

std::unique_ptr<Shader> VideoMetal::createShader(const ShaderDefinition&)
{
  return std::make_unique<MetalShader>();
}

std::unique_ptr<TextureRenderTarget> VideoMetal::createTextureRenderTarget()
{
  return std::make_unique<TextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> VideoMetal::createScreenRenderTarget()
{
  return std::make_unique<ScreenRenderTarget>(Rect4i({}, getWindow().getWindowRect().getSize()));
}

std::unique_ptr<MaterialConstantBuffer> VideoMetal::createConstantBuffer()
{
  return std::make_unique<MetalMaterialConstantBuffer>();
}

String VideoMetal::getShaderLanguage()
{
  return "glsl";
}

std::unique_ptr<Painter> VideoMetal::makePainter(Resources& resources)
{
  return std::make_unique<MetalPainter>(*this, resources);
}

id<CAMetalDrawable> VideoMetal::getSurface() {
  return surface;
}

id<MTLCommandQueue> VideoMetal::getCommandQueue() {
  return command_queue;
}

MetalTexture::MetalTexture(Vector2i size)
  : Texture(size)
{
}

void MetalTexture::load(TextureDescriptor&&)
{
  doneLoading();
}

int MetalShader::getUniformLocation(const String&, ShaderType)
{
  return 0;
}

int MetalShader::getBlockLocation(const String&, ShaderType)
{
  return 0;
}

void MetalMaterialConstantBuffer::update(const MaterialDataBlock&) {}


MetalPainter::MetalPainter(VideoMetal& video, Resources& resources)
  : Painter(resources)
  , video(video)
{}

void MetalPainter::clear(Colour colour) {}

void MetalPainter::setMaterialPass(const Material&, int) {}

void MetalPainter::doStartRender() {
  buffer = [video.getCommandQueue() commandBuffer];
  Colour col = Colour4f(0);
  descriptor = renderPassDescriptorForTextureAndColour(video.getSurface().texture, col);
  encoder = [buffer renderCommandEncoderWithDescriptor:descriptor];
}

void MetalPainter::doEndRender() {
  [encoder endEncoding];
  [buffer presentDrawable:video.getSurface()];
  [buffer commit];
  [encoder release];
  [buffer release];
  [descriptor release];
}

void MetalPainter::setVertices(const MaterialDefinition&, size_t, void*, size_t, unsigned short*, bool) {}

void MetalPainter::drawTriangles(size_t) {}

void MetalPainter::setViewPort(Rect4i) {}

void MetalPainter::setClip(Rect4i, bool) {}

void MetalPainter::setMaterialData(const Material&) {}

void MetalPainter::onUpdateProjection(Material&) {}
