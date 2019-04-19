#include "metal_material_constant_buffer.h"
#include "metal_painter.h"
#include "metal_render_target.h"
#include "metal_texture.h"
#include "metal_video.h"

#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/shader.h>
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
