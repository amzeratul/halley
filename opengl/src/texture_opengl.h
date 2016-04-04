#pragma once

namespace Halley
{
	class TextureOpenGL : public Texture
	{
	public:
		explicit TextureOpenGL(TextureDescriptor& descriptor);

	private:
		void create(size_t w, size_t h, int bpp, int format, bool useMipMap, bool useFiltering);
	};
}
