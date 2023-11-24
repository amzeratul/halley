#include "vulkan_video.h"

#include "volk/volk.h"

#include <SDL_vulkan.h>

// TODO VULKAN TYPES
#include "halley/graphics/painter.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/graphics/shader.h"

using namespace Halley;


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
	void* userData)
{
	switch (messageSeverity)
	{
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		Logger::logError(callbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		Logger::logWarning(callbackData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		Logger::logDev(callbackData->pMessage);
		break;
	}
	return VK_FALSE;
}


VulkanVideo::VulkanVideo(SystemAPI& system)
	: system(system)
{}

void VulkanVideo::init()
{
	volkInitialize();
}

void VulkanVideo::deInit()
{
	vkDestroyDebugUtilsMessengerEXT(instance, debugUtils, nullptr);
	vkDestroyInstance(instance, nullptr);
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
		initVulkan(*window);
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

void VulkanVideo::initVulkan(Window& window)
{
	createInstance(window);
	volkLoadInstance(instance);

	createDebugCallback();
}

void VulkanVideo::createInstance(Window& window)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Halley Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Halley";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	unsigned int sdlExtensionCount = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(static_cast<SDL_Window*>(window.getHandle()), &sdlExtensionCount, nullptr)) { // TODO MULTI PLATFORM
		throw Halley::Exception("SDL_Vulkan_GetInstanceExtensions fails!", Halley::HalleyExceptions::VideoPlugin);
	}

	std::vector<const char*> extensions(sdlExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(static_cast<SDL_Window*>(window.getHandle()), &sdlExtensionCount, extensions.data()); // TODO MULTI PLATFORM

	extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // TODO ONLY ACTIVE IN DEBUG

	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	const std::vector<const char*> validationLayers = { // TODO ONLY ACTIVE IN DEBUG
		"VK_LAYER_KHRONOS_validation"
	};

	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()); // TODO ONLY ACTIVE IN DEBUG
	createInfo.ppEnabledLayerNames = validationLayers.data();

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw Halley::Exception("Failed creating a vulkan instance!", Halley::HalleyExceptions::VideoPlugin);
	}
}

void VulkanVideo::createDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugUtils) != VK_SUCCESS) {
		throw Halley::Exception("Failed creating vulkan debug utils!", Halley::HalleyExceptions::VideoPlugin);
	}
}
