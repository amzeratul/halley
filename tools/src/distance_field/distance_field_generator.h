#pragma once

#include <memory>
#include <halley/maths/vector2d.h>

namespace Halley
{
	class Image;

	class DistanceFieldGenerator
	{
	public:
		static std::unique_ptr<Image> generate(Image& src, Vector2i size, float radius);
	};
}
