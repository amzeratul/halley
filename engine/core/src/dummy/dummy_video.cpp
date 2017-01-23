#include "dummy_video.h"
#include <halley/core/graphics/painter.h>
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
#include <halley/core/graphics/render_target/render_target_texture.h>
#include "dummy_system.h"

using namespace Halley;

DummyVideoAPI::DummyVideoAPI(SystemAPI& system) 
	: system(system)
{}

void DummyVideoAPI::startRender()
{
}

void DummyVideoAPI::finishRender()
{
}

void DummyVideoAPI::flip()
{
}

void DummyVideoAPI::setWindow(WindowDefinition&& windowDescriptor, bool vsync)
{
	//window = system.createWindow(windowDescriptor);
	window = std::make_shared<DummyWindow>(windowDescriptor);
}

const Window& DummyVideoAPI::getWindow() const
{
	return *window;
}

std::unique_ptr<Texture> DummyVideoAPI::createTexture(Vector2i size)
{
	return std::make_unique<DummyTexture>(size);
}

std::unique_ptr<Shader> DummyVideoAPI::createShader(String name)
{
	return std::make_unique<DummyShader>(name);
}

std::unique_ptr<TextureRenderTarget> DummyVideoAPI::createRenderTarget()
{
	return std::make_unique<DummyTextureRenderTarget>();
}

void DummyVideoAPI::init()
{
}

void DummyVideoAPI::deInit()
{
}

std::unique_ptr<Painter> DummyVideoAPI::makePainter()
{
	return std::make_unique<DummyPainter>();
}

std::function<void(int, void*)> DummyVideoAPI::getUniformBinding(UniformType type, int n)
{
	return [] (int, void*) {};
}

DummyTexture::DummyTexture(Vector2i s)
{
	size = s;
}

void DummyTexture::bind(int textureUnit) const {}

void DummyTexture::load(const TextureDescriptor& descriptor)
{
	doneLoading();
}

bool DummyTextureRenderTarget::isScreen() const
{
	return false;
}

void DummyTextureRenderTarget::bind() {}

void DummyTextureRenderTarget::unbind() {}

DummyShader::DummyShader(const String& name)
: Shader(name)
{}

void DummyShader::bind() {}

void DummyShader::compile() {}

void DummyShader::addVertexSource(String src) {}

void DummyShader::addGeometrySource(String src) {}

void DummyShader::addPixelSource(String src) {}

void DummyShader::setAttributes(const Vector<MaterialAttribute>& attributes) {}

unsigned DummyShader::getUniformLocation(String name)
{
	return 0;
}

unsigned DummyShader::getAttributeLocation(String name)
{
	return 0;
}

void DummyPainter::clear(Colour colour) {}

void DummyPainter::setBlend(BlendType blend) {}

void DummyPainter::doStartRender() {}

void DummyPainter::doEndRender() {}

void DummyPainter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices) {}

void DummyPainter::drawTriangles(size_t numIndices) {}

void DummyPainter::setViewPort(Rect4i rect, Vector2i renderTargetSize, bool isScreen) {}

void DummyPainter::setClip(Rect4i clip, Vector2i renderTargetSize, bool enable, bool isScreen) {}
