#include "dx11_video.h"
#include "dx11_painter.h"
#include "dx11_shader.h"
#include "dx11_texture.h"
#include "dx11_render_target.h"
#include "dx11_material_constant_buffer.h"

using namespace Halley;

DX11Video::DX11Video(SystemAPI& system)
	: system(system)
{}

void DX11Video::startRender() {}

void DX11Video::finishRender() {}

void DX11Video::setWindow(WindowDefinition&& windowDescriptor, bool vsync)
{
	if (!window) {
		window = system.createWindow(windowDescriptor);
	} else {
		window->update(windowDescriptor);
	}
}

const Window& DX11Video::getWindow() const
{
	return *window;
}

bool DX11Video::hasWindow() const
{
	return false;
}

std::unique_ptr<Texture> DX11Video::createTexture(Vector2i size)
{
	return std::make_unique<DX11Texture>(size);
}

std::unique_ptr<Shader> DX11Video::createShader(const ShaderDefinition& definition)
{
	return std::make_unique<DX11Shader>(definition);
}

std::unique_ptr<TextureRenderTarget> DX11Video::createTextureRenderTarget()
{
	return std::make_unique<DX11TextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> DX11Video::createScreenRenderTarget()
{
	return std::make_unique<DX11ScreenRenderTarget>(Rect4i(Vector2i(), window->getWindowRect().getSize()));
}

std::unique_ptr<MaterialConstantBuffer> DX11Video::createConstantBuffer()
{
	return std::make_unique<DX11MaterialConstantBuffer>();
}

void DX11Video::init() {}

void DX11Video::deInit() {}

std::unique_ptr<Painter> DX11Video::makePainter(Resources& resources)
{
	return std::make_unique<DX11Painter>(resources);
}
