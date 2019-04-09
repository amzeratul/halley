#include "video_metal.h"
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>

using namespace Halley;

///////////////
// Constructor
VideoMetal::VideoMetal(SystemAPI& system)
  : system(system)
{
}

void VideoMetal::init()
{
  std::cout << "Initializing Metal..." << std::endl;
  device = MTLCreateSystemDefaultDevice();
  std::cout << "Metal initialized with:" << std::endl;
  std::cout << "\tDevice: " << [device.name UTF8String] << std::endl;
}

void VideoMetal::deInit()
{
  std::cout << "Shutting down Metal..." << std::endl;
}

void VideoMetal::startRender()
{
}

void VideoMetal::finishRender()
{
}


void VideoMetal::setWindow(WindowDefinition&& windowDescriptor)
{
  window = system.createWindow(windowDescriptor);
}

const Window& VideoMetal::getWindow() const
{
  Expects(window);
  return *window;
}

bool VideoMetal::hasWindow() const
{
  return static_cast<bool>(window);
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
  return std::make_unique<MetalPainter>(resources);
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


MetalPainter::MetalPainter(Resources& resources)
  : Painter(resources)
{}

void MetalPainter::clear(Colour colour) {}

void MetalPainter::setMaterialPass(const Material&, int) {}

void MetalPainter::doStartRender() {}

void MetalPainter::doEndRender() {}

void MetalPainter::setVertices(const MaterialDefinition&, size_t, void*, size_t, unsigned short*, bool) {}

void MetalPainter::drawTriangles(size_t) {}

void MetalPainter::setViewPort(Rect4i) {}

void MetalPainter::setClip(Rect4i, bool) {}

void MetalPainter::setMaterialData(const Material&) {}

void MetalPainter::onUpdateProjection(Material&) {}
