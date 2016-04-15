#include "painter.h"
#include "render_context.h"
#include "render_target/render_target.h"

using namespace Halley;

void Painter::bind(RenderContext& context)
{
	context.getRenderTarget().bind();
}
