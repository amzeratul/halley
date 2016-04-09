#pragma once

#include "halley_gl.h"

namespace Halley
{
	class PainterOpenGL : public Painter
	{
	public:
		void startRender() override;
		void endRender() override;

		void drawSprite(Material& material, Vector2f pos) override;
		void clear(Colour colour) override;

	private:
		GLuint vbo;

		void draw(Material& material, Vector2f pos);
		void drawArraysQuads(int n);
		char* setupVBO(size_t size);
	};
}
