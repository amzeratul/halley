#pragma once
#include <memory>
#include <gsl/gsl>
#include <map>
#include "halley/text/halleystring.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"

namespace Halley
{
	class Path;
	class Image;

	struct ImageData
	{
		int frameNumber;
		int duration;
		String sequenceName;
		Rect4i clip;
		Vector2i pivot;
		Vector4s slices;

		std::unique_ptr<Image> img;
		std::vector<String> filenames;
	};

	class AsepriteReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, bool trim);

	private:
		static std::vector<ImageData> loadImagesFromPath(Path tmp, bool crop);
		static std::map<int, int> getSpriteDurations(Path jsonPath);
		static void processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations);
	};
}
