#include "vulkan_video.h"

#include "volk/volk.h"

// TODO VULKAN TYPES
#include "halley/graphics/painter.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/graphics/shader.h"

using namespace Halley;

VulkanVideo::VulkanVideo(SystemAPI& system)
	: system(system)
{}

void VulkanVideo::init()
{
	volkInitialize();
}

void VulkanVideo::deInit()
{
}

void VulkanVideo::onResume()
{
}

void VulkanVideo::onSuspend()
{
}

void VulkanVideo::startRender()
{
}

void VulkanVideo::finishRender()
{
}

void VulkanVideo::waitForVsync()
{
}

void VulkanVideo::setWindow(WindowDefinition&& windowDescriptor)
{
	if (!window) {
		window = system.createWindow(windowDescriptor);
	} else {
		window->update(windowDescriptor);
	}
}

Window& VulkanVideo::getWindow() const
{
	return *window;
}

bool VulkanVideo::hasWindow() const
{
	return !!window;
}

void VulkanVideo::setVsync(bool vsync)
{
}

bool VulkanVideo::hasVsync() const
{
	return useVsync;
}

std::unique_ptr<Texture> VulkanVideo::createTexture(Vector2i size)
{
	return nullptr; //std::make_unique<VulkanTexture>(*this, size);
}

std::unique_ptr<Shader> VulkanVideo::createShader(const ShaderDefinition& definition)
{
	return nullptr; //std::make_unique<VulkanShader>(*this, definition);
}

std::unique_ptr<TextureRenderTarget> VulkanVideo::createTextureRenderTarget()
{
	return nullptr; //std::make_unique<VulkanTextureRenderTarget>(*this);
}

std::unique_ptr<ScreenRenderTarget> VulkanVideo::createScreenRenderTarget()
{
	return nullptr; //std::make_unique<VulkanScreenRenderTarget>(*this, view);
}

std::unique_ptr<MaterialConstantBuffer> VulkanVideo::createConstantBuffer()
{
	return nullptr; //std::make_unique<VulkanMaterialConstantBuffer>(*this);
}

std::unique_ptr<Painter> VulkanVideo::makePainter(Resources& resources)
{
	return nullptr; //std::make_unique<VulkanPainter>(*this, resources);
}

String VulkanVideo::getShaderLanguage()
{
	return "hlsl";
}

SystemAPI& VulkanVideo::getSystem()
{
	return system;
}

void* VulkanVideo::getImplementationPointer(const String& id)
{
	//if (id == "ID3D11Device") {
	//	return static_cast<IUnknown*>(device);
	//}
	return nullptr;
}