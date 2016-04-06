#pragma once

namespace Halley
{
	class PainterOpenGL : public Painter
	{
	public:
		void startRender() override;
		void endRender() override;

		void drawSprite(Material& material, Vector2f pos) override;
		void clear(Colour colour) override;
	};
}
