#include "render_context.h"
#include "render_target/render_target.h"

using namespace Halley;

RenderContext::RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget)
	: painter(painter)
	, camera(camera)
	, renderTarget(renderTarget)
{}

void RenderContext::setActive()
{
	painter.activeContext = this;
	painter.bind(camera, renderTarget);
}

void RenderContext::pushContext()
{
	restore = painter.activeContext;
}

void RenderContext::popContext()
{
	if (restore) {
		restore->setActive();
		restore = nullptr;
	}
}

RenderContext RenderContext::with(Camera& v) const
{
	return RenderContext(painter, v, renderTarget);
}

RenderContext RenderContext::with(RenderTarget& v) const
{
	return RenderContext(painter, camera, v);
}

RenderContext::RenderContext(RenderContext&& context)
	: painter(context.painter)
	, camera(context.camera)
	, renderTarget(context.renderTarget)
{
}

RenderContext RenderContext::subArea(Rect4f area) const
{
	// TODO
	//return RenderContext(painter, camera, renderTarget.makeSubArea(area));
	return RenderContext(painter, camera, renderTarget);
}
