#include "render_target_opengl.h"

using namespace Halley;

Vector2f RenderTargetOpenGL::getSize() const
{
	return Vector2f(attachments[0]->getSize());
}

Rect4f RenderTargetOpenGL::getViewPort() const
{
	return Rect4f(Vector2f(0, 0), getSize());
}

void RenderTargetOpenGL::bind()
{
	// TODO
}

void RenderTargetOpenGL::unbind()
{
	// TODO
}
