#pragma once

namespace Halley
{
	class Material;

	class Painter
	{
	public:
		virtual ~Painter() {}

		void drawSprite(Material& material, Vector2f pos);
	};
}
