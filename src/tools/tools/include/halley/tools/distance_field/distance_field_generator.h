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
		enum class Type {
			SDF,
			MTSDF
		};
		static std::unique_ptr<Image> generateSDF(Image& src, Vector2i size, float radius);
		static std::unique_ptr<Image> generateMSDF(Type type, const FontFace& font, float fontSize, int charcode, Vector2i size, float radius);
	};
}
