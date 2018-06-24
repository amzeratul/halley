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

	class AsepriteExternalReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);

	private:
		static std::vector<ImageData> loadImagesFromPath(Path tmp, bool crop);
		static std::map<int, int> getSpriteDurations(Path jsonPath);
		static void processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations);
	};

	class AsepriteReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);
	};
}
