#pragma once

namespace Halley
{
	class Material;

	class Painter
	{
	public:
		virtual ~Painter() {}

		virtual void startRender() = 0;
		virtual void endRender() = 0;

		virtual void drawSprite(Material& material, Vector2f pos) = 0;
		virtual void clear(Colour colour) = 0;
	};
}
