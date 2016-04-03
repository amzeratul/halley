#pragma once

namespace Halley
{
	class PainterOpenGL : public Painter
	{
	public:
		void drawSprite(Material& material, Vector2f pos) override;
		void clear(Colour colour) override;
	};
}