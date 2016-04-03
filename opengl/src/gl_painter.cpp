#include "gl_painter.h"
#include <GL/glew.h>
#include <GL/GL.h>
#include "../../core/src/graphics/material.h"

void Halley::PainterOpenGL::drawSprite(Material& material, Vector2f pos)
{
	material.ensureLoaded();

	// TODO
}

void Halley::PainterOpenGL::clear(Colour colour)
{
	glClearColor(colour.r, colour.g, colour.b, colour.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
