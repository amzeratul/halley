#pragma once

#include "halley/data_structures/maybe.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class Image;

	class IClipboard
	{
	public:
		virtual ~IClipboard() = default;
		
		virtual void setData(const String& stringData) = 0;
		virtual std::optional<String> getStringData() = 0;

		virtual void setData(const Image& image) = 0;
		virtual std::unique_ptr<Image> getImageData() = 0;
	};
}
