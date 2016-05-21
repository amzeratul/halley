#pragma once

#include <halley/maths/vector2d.h>

namespace Halley
{
	enum class TextureFormat
	{
		RGBA,
		RGB,
		DEPTH
	};

	class TextureDescriptor
	{
	public:
		Vector2i size;
		size_t padding = 0;
		TextureFormat format = TextureFormat::RGBA;
		bool useMipMap = false;
		bool useFiltering = false;
		void* pixelData = nullptr;

		static int getBitsPerPixel(TextureFormat format);

		TextureDescriptor() {}
		TextureDescriptor(Vector2i size, TextureFormat format = TextureFormat::RGBA)
			: size(size)
			, format(format)
		{}
	};
}
