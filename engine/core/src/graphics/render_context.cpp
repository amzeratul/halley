#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"

using namespace Halley;

RenderContext::RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget, Rect4i viewPort)
	: painter(painter)
	, camera(camera)
	, renderTarget(renderTarget)
	, viewPort(viewPort)
{}

RenderContext::RenderContext(RenderContext&& context)
	: painter(context.painter)
	, camera(context.camera)
	, renderTarget(context.renderTarget)
	, viewPort(context.viewPort)
{
}

void RenderContext::setActive()
{
	painter.activeContext = this;
	painter.bind(*this);
}

void RenderContext::pushContext()
{
	restore = painter.activeContext;
}

void RenderContext::popContext()
{
	painter.flush();
	if (restore) {
		restore->setActive();
		restore = nullptr;
	}
}

RenderContext RenderContext::with(Camera& v) const
{
	return RenderContext(painter, v, renderTarget, viewPort);
}

RenderContext RenderContext::with(RenderTarget& v) const
{
	return RenderContext(painter, camera, v, viewPort);
}

RenderContext RenderContext::subArea(Rect4i area) const
{
	Vector2i start = viewPort.getTopLeft() + area.getTopLeft();
	Vector2i end = start + area.getSize();
	end.x = std::min(end.x, viewPort.getBottomRight().x);
	end.y = std::min(end.y, viewPort.getBottomRight().y);
	return RenderContext(painter, camera, renderTarget, Rect4i(start, end));
}
