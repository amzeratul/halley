#pragma once
#include "api/halley_api_internal.h"
#include "graphics/texture.h"
#include "graphics/render_target/render_target_texture.h"
#include "graphics/shader.h"
#include "graphics/painter.h"

namespace Halley {
	class DummyVideoAPI : public VideoAPIInternal {
	public:
		explicit DummyVideoAPI(SystemAPI& system);

		void startRender() override;
		void finishRender() override;
		void setWindow(WindowDefinition&& windowDescriptor, bool vsync) override;
		const Window& getWindow() const override;
		std::unique_ptr<Texture> createTexture(Vector2i size) override;
		std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) override;
		std::unique_ptr<TextureRenderTarget> createRenderTarget() override;
		void init() override;
		void deInit() override;
		std::unique_ptr<Painter> makePainter() override;
		std::function<void(int, void*)> getUniformBinding(UniformType type, int n) override;

	private:
		SystemAPI& system;
		std::shared_ptr<Window> window;
	};

	class DummyTexture : public Texture
	{
	public:
		explicit DummyTexture(Vector2i size);
		void bind(int textureUnit) const override;
		void load(TextureDescriptor&& descriptor) override;
	};

	class DummyTextureRenderTarget : public TextureRenderTarget
	{
	public:
		bool isScreen() const override;
		void bind() override;
		void unbind() override;
	};

	class DummyShader : public Shader
	{
	public:
		unsigned getUniformLocation(String name) override;
	};

	class DummyMaterialConstantBuffer : public MaterialConstantBuffer
	{
	public:
		void update(const Vector<MaterialParameter>& uniforms) override;
		void bind(int pass) override;
	};

	class DummyPainter : public Painter
	{
	public:
		void clear(Colour colour) override;
		void setBlend(BlendType blend) override;
		void doStartRender() override;
		void doEndRender() override;
		void setVertices(const MaterialDefinition& material, size_t numVertices, void* vertexData, size_t numIndices, unsigned short* indices) override;
		void drawTriangles(size_t numIndices) override;
		void setViewPort(Rect4i rect, Vector2i renderTargetSize, bool isScreen) override;
		void setClip(Rect4i clip, Vector2i renderTargetSize, bool enable, bool isScreen) override;
		void setShader(Shader& shader) override;
		std::unique_ptr<MaterialConstantBuffer> makeConstantBuffer(const Material& material) override;
	};
}
