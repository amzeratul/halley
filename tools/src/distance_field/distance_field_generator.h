#pragma once

namespace Halley
{
	class DistanceFieldGenerator
	{
	public:
		static std::unique_ptr<Image> generate(Image& src, Vector2i size, float radius);
	};
}
