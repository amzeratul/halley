#pragma once

namespace Halley
{
	enum class TextureFormat
	{
		RGBA
	};

	class TextureDescriptor
	{
	public:
		size_t w = 0;
		size_t h = 0;
		size_t padding = 0;
		TextureFormat format = TextureFormat::RGBA;
		void* pixelData = nullptr;
	};
}
