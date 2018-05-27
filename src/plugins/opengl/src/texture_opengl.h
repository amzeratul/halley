#pragma once

#include <halley/core/graphics/texture.h>
#include <halley/core/graphics/texture_descriptor.h>
#include "halley_gl.h"

namespace Halley
{
	class VideoOpenGL;
	enum class TextureFormat;

	class TextureOpenGL final : public Texture
	{
	public:
		explicit TextureOpenGL(VideoOpenGL& parent, Vector2i size);
		~TextureOpenGL();

		TextureOpenGL& operator=(TextureOpenGL&& other) noexcept;

		void bind(int textureUnit) const;
		unsigned int getNativeId() const;

		void load(TextureDescriptor&& descriptor) override;
		void reload(Resource&& resource) override;

	private:
		void updateImage(TextureDescriptorImageData& pixelData, TextureFormat format, bool useMipMap);
		void create(Vector2i size, TextureFormat format, bool useMipMap, bool useFiltering, bool clamp, TextureDescriptorImageData& imgData);

		static unsigned int getGLFormat(TextureFormat format);

		void waitForOpenGLLoad() const;
		void finishLoading();

		unsigned int textureId = 0;
		Vector2i texSize;
		VideoOpenGL& parent;
#ifdef WITH_OPENGL
		mutable GLsync fence = nullptr;
#endif
	};
}
