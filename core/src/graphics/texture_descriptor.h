#pragma once

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
		size_t w = 0;
		size_t h = 0;
		size_t padding = 0;
		TextureFormat format = TextureFormat::RGBA;
		bool useMipMap = false;
		bool useFiltering = false;
		void* pixelData = nullptr;

		static int getBitsPerPixel(TextureFormat format);
	};
}
