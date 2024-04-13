#include "dummy_video.h"
#include <halley/graphics/painter.h>
#include <halley/graphics/texture.h>
#include <halley/graphics/shader.h>
#include <halley/graphics/render_target/render_target_texture.h>
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

void DummyVideoAPI::setWindow(WindowDefinition&& windowDescriptor)
{
	//window = system.createWindow(windowDescriptor);
	window = std::make_shared<DummyWindow>(windowDescriptor);
}

Window& DummyVideoAPI::getWindow() const
{
	Expects(window != nullptr);
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

std::unique_ptr<MaterialShaderStorageBuffer> DummyVideoAPI::createShaderStorageBuffer()
{
	return std::make_unique<DummyMaterialShaderStorageBuffer>();
}

void DummyVideoAPI::init()
{
}

void DummyVideoAPI::deInit()
{
}

std::unique_ptr<Painter> DummyVideoAPI::makePainter(Resources& resources)
{
	return std::make_unique<DummyPainter>(*this, resources);
}

String DummyVideoAPI::getShaderLanguage()
{
	return "glsl";
}

DummyTexture::DummyTexture(Vector2i size)
	: Texture(size)
{
}

void DummyTexture::doLoad(TextureDescriptor&)
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

void DummyMaterialConstantBuffer::update(gsl::span<const gsl::byte> data) {}

void DummyMaterialShaderStorageBuffer::update(size_t numElements, size_t pitch, gsl::span<const gsl::byte> data) {}

void DummyMaterialShaderStorageBuffer::bind(ShaderType type, int position) {}

DummyPainter::DummyPainter(VideoAPI& video, Resources& resources)
	: Painter(video, resources)
{}

void DummyPainter::doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil) {}

void DummyPainter::setMaterialPass(const Material&, int) {}

void DummyPainter::doStartRender() {}

void DummyPainter::doEndRender() {}

void DummyPainter::setVertices(const MaterialDefinition&, size_t, const void*, size_t, const IndexType*, bool) {}

void DummyPainter::drawTriangles(size_t) {}

void DummyPainter::setViewPort(Rect4i) {}

void DummyPainter::setClip(Rect4i, bool) {}

void DummyPainter::setMaterialData(const Material&) {}

void DummyPainter::onUpdateProjection(Material&, bool) {}
