#pragma once
#include <gsl/gsl>
#include <map>
#include "halley/text/halleystring.h"
#include "halley/maths/vector4.h"

namespace Halley
{
	class Path;
	class Image;
	struct ImageData;

	class AsepriteReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);
	};
}
