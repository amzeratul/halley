#pragma once

namespace Halley
{
	class Material;

	class Painter
	{
	public:
		virtual ~Painter() {}

		virtual void drawSprite(Material& material, Vector2f pos) = 0;
		virtual void clear(Colour colour) = 0;
	};
}
