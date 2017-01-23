#include "dummy_video.h"
#include <halley/core/graphics/painter.h>
#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
#include <halley/core/graphics/render_target/render_target_texture.h>

using namespace Halley;

PluginType DummyVideoPlugin::getType()
{
	return PluginType::GraphicsAPI;
}

String DummyVideoPlugin::getName()
{
	return "dummyVideo";
}

HalleyAPIInternal* DummyVideoPlugin::createAPI(SystemAPI*)
{
	return new DummyVideoAPI();
}

int DummyVideoPlugin::getPriority() const
{
	return -1;
}

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
}

const Window& DummyVideoAPI::getWindow() const
{
	return *window;
}

std::unique_ptr<Texture> DummyVideoAPI::createTexture(Vector2i size)
{
	return {};
}

std::unique_ptr<Shader> DummyVideoAPI::createShader(String name)
{
	return {};
}

std::unique_ptr<TextureRenderTarget> DummyVideoAPI::createRenderTarget()
{
	return {};
}

void DummyVideoAPI::init()
{
}

void DummyVideoAPI::deInit()
{
}

std::unique_ptr<Painter> DummyVideoAPI::makePainter()
{
	return {};
}

std::function<void(int, void*)> DummyVideoAPI::getUniformBinding(UniformType type, int n)
{
	return {};
}
