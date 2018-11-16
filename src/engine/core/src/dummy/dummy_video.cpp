#include "dummy_video.h"
#include <halley/core/graphics/painter.h>
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
#include <halley/core/graphics/render_target/render_target_texture.h>
#include "dummy_system.h"
#include <chrono>

using namespace Halley;

DummyVideoAPI::DummyVideoAPI(SystemAPI&) 
{}

void DummyVideoAPI::startRender()
{
}

void DummyVideoAPI::finishRender()
{
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(10ms);
}

void DummyVideoAPI::setWindow(WindowDefinition&& windowDescriptor)
{
	//window = system.createWindow(windowDescriptor);
	window = std::make_shared<DummyWindow>(windowDescriptor);
}

const Window& DummyVideoAPI::getWindow() const
{
	Expects(window);
	return *window;
}

bool DummyVideoAPI::hasWindow() const
{
	return static_cast<bool>(window);
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
	return std::make_unique<TextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> DummyVideoAPI::createScreenRenderTarget()
{
	return std::make_unique<ScreenRenderTarget>(Rect4i({}, getWindow().getWindowRect().getSize()));
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

String DummyVideoAPI::getShaderLanguage()
{
	return "glsl";
}

DummyTexture::DummyTexture(Vector2i size)
	: Texture(size)
{
}

void DummyTexture::load(TextureDescriptor&&)
{
	doneLoading();
}

int DummyShader::getUniformLocation(const String&, ShaderType)
{
	return 0;
}

int DummyShader::getBlockLocation(const String&, ShaderType)
{
	return 0;
}

void DummyMaterialConstantBuffer::update(const MaterialDataBlock&) {}

DummyPainter::DummyPainter(Resources& resources)
	: Painter(resources)
{}

void DummyPainter::clear(Colour colour) {}

void DummyPainter::setMaterialPass(const Material&, int) {}

void DummyPainter::doStartRender() {}

void DummyPainter::doEndRender() {}

void DummyPainter::setVertices(const MaterialDefinition&, size_t, void*, size_t, unsigned short*, bool) {}

void DummyPainter::drawTriangles(size_t) {}

void DummyPainter::setViewPort(Rect4i) {}

void DummyPainter::setClip(Rect4i, bool) {}

void DummyPainter::setMaterialData(const Material&) {}

void DummyPainter::onUpdateProjection(Material&) {}
