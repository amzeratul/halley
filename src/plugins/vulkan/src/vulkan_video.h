#pragma once

#include "halley/api/halley_api_internal.h"
#include "halley/graphics/window.h"

#include "vulkan/vulkan.h"

namespace Halley {
	class SystemAPI;

	class VulkanVideo final : public VideoAPIInternal
	{
	public:
		VulkanVideo(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		void waitForVsync() override;
		
		void setWindow(WindowDefinition&& windowDescriptor) override;
		Window& getWindow() const override;
		bool hasWindow() const override;
		void setVsync(bool vsync) override;
		bool hasVsync() const override;
		
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		
		void init() override;
		void deInit() override;
		void onResume() override;
		void onSuspend() override;

		std::unique_ptr<Painter> makePainter(Resources& resources) override;

		String getShaderLanguage() override;

		SystemAPI& getSystem();

		void* getImplementationPointer(const String& id) override;

	private:
		SystemAPI& system;
		std::shared_ptr<Window> window;

		void initVulkan(Window& window);
		void createInstance(Window& window);
		void createDebugCallback();

		VkInstance instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT debugUtils = VK_NULL_HANDLE;

		bool useVsync = false;
	};
}
