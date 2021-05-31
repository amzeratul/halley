#pragma once
#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>
#include <halley/text/halleystring.h>
#include <halley/file/path.h>

namespace Halley
{
	class ShaderDefinition;
	class WindowDefinition;
	class Painter;
	class Texture;
	class TextureDescriptor;
	class TextureRenderTarget;
	class ScreenRenderTarget;
	class Shader;
	class Window;
	class MaterialConstantBuffer;
	class Material;

	class VideoAPI
	{
	public:
		virtual ~VideoAPI() {}

		virtual void startRender() = 0;
		virtual void finishRender() = 0;

		virtual void setWindow(WindowDefinition&& windowDescriptor) = 0;
		virtual Window& getWindow() const = 0;
		virtual bool hasWindow() const = 0;
		virtual void setVsync(bool vsync) {}

		virtual std::unique_ptr<Texture> createTexture(Vector2i size) = 0;
		virtual std::unique_ptr<Shader> createShader(const ShaderDefinition& definition) = 0;
		virtual std::unique_ptr<TextureRenderTarget> createTextureRenderTarget() = 0;
		virtual std::unique_ptr<ScreenRenderTarget> createScreenRenderTarget() = 0;
		virtual std::unique_ptr<MaterialConstantBuffer> createConstantBuffer() = 0;

		virtual String getShaderLanguage() = 0;
		virtual bool isColumnMajor() const { return false; }

		virtual void* getImplementationPointer(const String& id) { return nullptr; }
	};
}
