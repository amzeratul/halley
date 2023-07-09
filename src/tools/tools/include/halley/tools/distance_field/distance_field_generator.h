#pragma once

#include <memory>
#include <halley/maths/vector2.h>

namespace Halley
{
	class Image;

	class DistanceFieldGenerator
	{
	public:
		static std::unique_ptr<Image> generateSDF(Image& src, Vector2i size, float radius);
		static std::unique_ptr<Image> generateMSDF(Image& src, Vector2i size, float radius);
	};
}
