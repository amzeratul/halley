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
		SystemAPI& system;
		std::shared_ptr<Window> window;
	};

	class DummyTexture : public Texture
	{
	public:
		explicit DummyTexture(Vector2i size);
		void bind(int textureUnit) const override;
		void load(const TextureDescriptor& descriptor) override;
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
		explicit DummyShader(const String& name);

		void bind() override;
		void compile() override;
		void addVertexSource(String src) override;
		void addGeometrySource(String src) override;
		void addPixelSource(String src) override;
		void setAttributes(const Vector<MaterialAttribute>& attributes) override;
		unsigned getUniformLocation(String name) override;
		unsigned getAttributeLocation(String name) override;
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
	};
}
