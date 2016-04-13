#include "painter.h"
#include "render_target/render_target.h"

using namespace Halley;

void Painter::bind(Camera& camera, RenderTarget& renderTarget)
{
	renderTarget.bind();
}
