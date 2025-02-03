#pragma once

#include <memory>
#include <halley/maths/vector2.h>

#include "halley/tools/make_font/font_face.h"

namespace msdfgen {
	class FontHandle;
	class Shape;
}

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

		struct ShapeData {
			
		};

		static std::unique_ptr<Image> generateSDF(Image& src, Vector2i size, float radius);
		static std::optional<msdfgen::Shape> generateMSDFShape(const FontFace& font, int charcode);
		static std::unique_ptr<Image> generateMSDF(Type type, const FontFace& font, float fontSize, int charcode, Vector2i size, float radius);
		static std::unique_ptr<Image> generateMSDF(Type type, msdfgen::Shape shape, float scale, Vector2i size, float radius);
		static float getScale(const FontFace& font, float fontSize);
	};
}
