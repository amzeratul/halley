#include "dummy_video.h"
#include <halley/core/graphics/painter.h>
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
#include <halley/core/graphics/render_target/render_target_texture.h>
#include "dummy_system.h"

using namespace Halley;

DummyVideoAPI::DummyVideoAPI(SystemAPI&) 
{}

void DummyVideoAPI::startRender()
{
}

void DummyVideoAPI::finishRender()
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

std::unique_ptr<Shader> DummyVideoAPI::createShader(const ShaderDefinition&)
{
	return std::make_unique<DummyShader>();
}

std::unique_ptr<TextureRenderTarget> DummyVideoAPI::createTextureRenderTarget()
{
	return std::make_unique<DummyTextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> DummyVideoAPI::createScreenRenderTarget()
{
	return std::make_unique<DummyScreenRenderTarget>(Rect4i({}, getWindow().getWindowRect().getSize()));
}

std::unique_ptr<MaterialConstantBuffer> DummyVideoAPI::createConstantBuffer()
{
	return std::make_unique<DummyMaterialConstantBuffer>();
}

void DummyVideoAPI::init()
{
}

void DummyVideoAPI::deInit()
{
}

std::unique_ptr<Painter> DummyVideoAPI::makePainter(Resources& resources)
{
	return std::make_unique<DummyPainter>(resources);
}

DummyTexture::DummyTexture(Vector2i s)
{
	size = s;
}

void DummyTexture::load(TextureDescriptor&& descriptor)
{
	doneLoading();
}

void DummyTextureRenderTarget::bind() {}
void DummyTextureRenderTarget::unbind() {}

DummyScreenRenderTarget::DummyScreenRenderTarget(Rect4i viewPort)
	: ScreenRenderTarget(viewPort)
{}

void DummyScreenRenderTarget::bind() {}
void DummyScreenRenderTarget::unbind() {}

int DummyShader::getUniformLocation(const String& name, ShaderType stage)
{
	return 0;
}

int DummyShader::getBlockLocation(const String& name, ShaderType stage)
{
	return 0;
}

void DummyMaterialConstantBuffer::update(const MaterialDataBlock& dataBlock) {}

DummyPainter::DummyPainter(Resources& resources)
	: Painter(resources)
{}

void DummyPainter::clear(Colour colour) {}

void DummyPainter::setMaterialPass(const Material& material, int pass) {}

void DummyPainter::doStartRender() {}

void DummyPainter::doEndRender() {}

void DummyPainter::setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) {}

void DummyPainter::drawTriangles(size_t numIndices) {}

void DummyPainter::setViewPort(Rect4i rect) {}

void DummyPainter::setClip(Rect4i clip, bool enable) {}

void DummyPainter::setMaterialData(const Material& material) {}
