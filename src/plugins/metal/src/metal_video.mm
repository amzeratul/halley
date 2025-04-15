#include "metal_material_constant_buffer.h"
#include "metal_painter.h"
#include "metal_render_target.h"
#include "metal_texture.h"
#include "metal_video.h"

#include <halley/graphics/texture.h>
#include <halley/graphics/shader.h>
#if __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_metal.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_metal.h>

#endif
#include <iostream>

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
	purgeBuffers();
	pool = [[NSAutoreleasePool alloc] init];
	surface = [swap_chain nextDrawable];
	command_buffer = [command_queue commandBuffer];
}

void MetalVideo::finishRender()
{
	[command_buffer presentDrawable:surface];
	[command_buffer commit];
	window->swap();
	[pool release];
}


void MetalVideo::setWindow(WindowDefinition&& windowDescriptor)
{
	if( window )
	{
		system.destroyWindow( window );
	}
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

Window& MetalVideo::getWindow() const
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
	return std::make_unique<MetalTextureRenderTarget>();
}

std::unique_ptr<ScreenRenderTarget> MetalVideo::createScreenRenderTarget()
{
	auto & window = getWindow();
	if (window.getNativeHandleType() != "SDL") {
		throw Exception("Only SDL2 windows are supported by Metal", HalleyExceptions::VideoPlugin);
	}

	Vector2i size;
	SDL_Window* sdl_window = static_cast<SDL_Window*>(window.getNativeHandle());
	SDL_Metal_GetDrawableSize(sdl_window, &size.x, &size.y);

	Vector2i logical_size = window.getWindowRect().getSize();

	int scale_factor = size.x / logical_size.x;

	// :TODO: Make sure size.y / logical_size.y is the same
	return std::make_unique<MetalScreenRenderTarget>(*this, Rect4i({}, logical_size), scale_factor);
}

std::unique_ptr<MaterialConstantBuffer> MetalVideo::createConstantBuffer()
{
	return std::make_unique<MetalMaterialConstantBuffer>(*this);
}

String MetalVideo::getShaderLanguage()
{
	return "metal";
}

bool MetalVideo::isColumnMajor() const
{
	return true;
}

std::unique_ptr<Painter> MetalVideo::makePainter(Resources& resources)
{
	return std::make_unique<MetalPainter>(*this, resources);
}

id<CAMetalDrawable> MetalVideo::getSurface() {
	return surface;
}

id<MTLDevice> MetalVideo::getDevice() {
	return device;
}

id<MTLCommandBuffer> MetalVideo::getCommandBuffer() {
	return command_buffer;
}

void MetalVideo::addBufferToRelease( id<MTLBuffer> buffer )
{
	bufferToRelease.push_back(buffer);
}

void MetalVideo::purgeBuffers()
{
	for(auto & buffer : bufferToRelease)
	{
		[buffer setPurgeableState:MTLPurgeableStateEmpty];
		[buffer release];
	}
	bufferToRelease.clear();
}
