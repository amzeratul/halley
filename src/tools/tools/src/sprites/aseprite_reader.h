#pragma once
#include <gsl/gsl>
#include <map>
#include <optional>


#include "aseprite_file.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/text/halleystring.h"
#include "halley/maths/vector4.h"

namespace Halley
{
	class Path;
	class Image;

	class AsepriteReader
	{
	public:
		static std::map<String, Vector<SpriteSheet::ImageData>> importAseprite(const String& spriteName, const Path& filename, gsl::span<const gsl::byte> fileData, bool trim, int padding, bool groupSeparated, bool sequenceSeparated);
		static void addImageData(int tagFrameNumber, int origFrameNumber, Vector<SpriteSheet::ImageData>& frameData, std::unique_ptr<Image> frameImage,
		                  const AsepriteFile& aseFile, const String& baseName, const String& sequence, const String& direction,
						  int duration, bool trim, int padding, bool hasFrameNumber, std::optional<String> group, bool firstImage,
		                  const String& spriteName, const Path& filename);
	};
}
