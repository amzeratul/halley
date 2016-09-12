#pragma once

#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/texture_descriptor.h>
#include <condition_variable>

namespace Halley
{
	class VideoOpenGL;
	enum class TextureFormat;

	class TextureOpenGL final : public Texture
	{
	public:
		explicit TextureOpenGL(VideoOpenGL& parent, Vector2i size);
		~TextureOpenGL();

		void bind(int textureUnit) override;
		void load(const TextureDescriptor& descriptor) override;

	private:
		void loadImage(const char* px, size_t w, size_t h, size_t stride, TextureFormat format, bool useMipMap);
		void create(size_t w, size_t h, TextureFormat format, bool useMipMap, bool useFiltering);

		static unsigned int getGLFormat(TextureFormat format);

		void waitForOpenGLLoad();

		VideoOpenGL& parent;
		GLsync fence = nullptr;
	};
}
