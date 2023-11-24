#pragma once

#include <halley/graphics/texture.h>
#include <halley/graphics/texture_descriptor.h>
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

		void doLoad(TextureDescriptor& descriptor) override;
		void reload(Resource&& resource) override;

		void generateMipMaps() override;

	private:
		void updateImage(TextureDescriptorImageData& pixelData, TextureFormat format, bool useMipMap);
		void create(Vector2i size, TextureFormat format, bool useMipMap, bool useFiltering, TextureAddressMode addressMode, TextureDescriptorImageData& imgData);

		static int getGLInternalFormat(TextureFormat format);
		static unsigned int getGLPixelFormat(TextureFormat format);
		static unsigned int getGLByteFormat(TextureFormat format);
		static unsigned int getGLAddressMode(TextureAddressMode addressMode);

		void waitForOpenGLLoad() const;
		void finishLoading();

		unsigned int textureId = 0;
		Vector2i texSize;
		VideoOpenGL& parent;
		mutable GLsync fence = nullptr;
	};
}
