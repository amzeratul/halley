#pragma once
#include "api/halley_api_internal.h"
#include "graphics/texture.h"
#include "graphics/render_target/render_target_texture.h"
#include "graphics/render_target/render_target_screen.h"
#include "graphics/shader.h"
#include "graphics/painter.h"

namespace Halley {
	class DummyVideoAPI : public VideoAPIInternal {
	public:
		explicit DummyVideoAPI(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		void setWindow(WindowDefinition&& windowDescriptor) override;
		Window& getWindow() const override;
		bool hasWindow() const override;
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() override;
		std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() override;
		std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() override;
		void init() override;
		void deInit() override;
		std::unique_ptr<Painter> makePainter(Resources& resources) override;
		String getShaderLanguage() override;

	private:
		std::shared_ptr<Window> window;
	};

	class DummyTexture : public Texture
	{
	public:
		explicit DummyTexture(Vector2i size);
		void doLoad(TextureDescriptor& descriptor) override;
	};

	class DummyShader : public Shader
	{
	public:
		int getUniformLocation(const String& name, ShaderType stage) override;
		int getBlockLocation(const String& name, ShaderType stage) override;
	};

	class DummyMaterialConstantBuffer : public MaterialConstantBuffer
	{
	public:
		void update(gsl::span<const gsl::byte> data) override;
	};

	class DummyPainter : public Painter
	{
	public:
		explicit DummyPainter(VideoAPI& video, Resources& resources);
		void clear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil) override;
		void setMaterialPass(const Material& material, int pass) override;
		void doStartRender() override;
		void doEndRender() override;
		void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices, bool standardQuadsOnly) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect) override;
		void setClip(Rect4i clip, bool enable) override;
		void setMaterialData(const Material& material) override;
		void onUpdateProjection(Material& material, bool hashChanged) override;
	};
}
