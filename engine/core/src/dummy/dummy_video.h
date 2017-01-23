#pragma once
#include "halley/plugin/plugin.h"
#include "api/halley_api_internal.h"

namespace Halley {
	class DummyVideoPlugin : public Plugin {
	public:
		PluginType getType() override;
		String getName() override;
		HalleyAPIInternal* createAPI(SystemAPI*) override;
		int getPriority() const override;
	};

	class DummyVideoAPI : public VideoAPIInternal {
	public:
		void startRender() override;
		void finishRender() override;
		void flip() override;
		void setWindow(WindowDefinition&& windowDescriptor, bool vsync) override;
		const Window& getWindow() const override;
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(String name) override;
		std::unique_ptr<TextureRenderTarget> createRenderTarget() override;
		void init() override;
		void deInit() override;
		std::unique_ptr<Painter> makePainter() override;
		std::function<void(int, void*)> getUniformBinding(UniformType type, int n) override;

	private:
		std::unique_ptr<Window> window;
	};
}
