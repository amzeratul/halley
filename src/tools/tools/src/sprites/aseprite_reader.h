#pragma once
#include <memory>
#include <gsl/gsl>
#include <map>
#include "halley/text/halleystring.h"
#include "halley/maths/rect.h"

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
	};

	class AsepriteReader
	{
	public:
		static std::vector<ImageData> importAseprite(String baseName, gsl::span<const gsl::byte> fileData, Vector2i pivot);

	private:
		static std::vector<ImageData> loadImagesFromPath(Path tmp, Vector2i pivot);
		static std::map<int, int> getSpriteDurations(Path jsonPath);
		static void processFrameData(String baseName, std::vector<ImageData>& frameData, std::map<int, int> durations);
	};
}
