#pragma once

#include <memory>
#include <halley/maths/vector2.h>

#include "halley/tools/make_font/font_face.h"

namespace Halley
{
	class Image;

	class DistanceFieldGenerator
	{
	public:
		static std::unique_ptr<Image> generateSDF(Image& src, Vector2i size, float radius);
		static std::unique_ptr<Image> generateMSDF(Image& src, Vector2i size, float radius);
		static std::unique_ptr<Image> generateSDF2(const FontFace& font, int charcode, Vector2i size, float radius);
	};
}
