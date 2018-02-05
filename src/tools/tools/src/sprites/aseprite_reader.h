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
		String filename;
		std::unique_ptr<Image> img;
		Rect4i clip;
		Vector2i pivot;
		Vector4s slices;
	};

	class AsepriteReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData);

	private:
		static std::vector<ImageData> loadImagesFromPath(Path tmp);
		static std::map<int, int> getSpriteDurations(Path jsonPath);
		static void processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations);
	};
}
