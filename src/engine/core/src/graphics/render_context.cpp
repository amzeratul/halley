#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_target.h"

using namespace Halley;

RenderContext::RenderContext(Painter& painter, const Camera& camera, RenderTarget& renderTarget)
	: painter(painter)
	, camera(camera)
	, defaultRenderTarget(renderTarget)
{}

RenderContext::RenderContext(const RenderContext& context) noexcept
	: painter(context.painter)
	, camera(context.camera)
	, defaultRenderTarget(context.defaultRenderTarget)
{
}

RenderContext::RenderContext(RenderContext&& context) noexcept
	: painter(context.painter)
	, camera(context.camera)
	, defaultRenderTarget(context.defaultRenderTarget)
{
}

void RenderContext::setActive()
{
	painter.activeContext = this;
	painter.bind(*this);
}

void RenderContext::setInactive()
{
	painter.unbind(*this);
	painter.activeContext = nullptr;
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

RenderContext RenderContext::with(const Camera& v) const
{
	return RenderContext(painter, v, defaultRenderTarget);
}

RenderContext RenderContext::with(RenderTarget& renderTarget) const
{
	return RenderContext(painter, camera, renderTarget);
}

RenderTarget& RenderContext::getDefaultRenderTarget() const
{
	return defaultRenderTarget;
}

void RenderContext::flush()
{
	painter.flush();
}

/*
RenderContext RenderContext::subArea(Rect4i area) const
{
	Vector2i start = viewPort.getTopLeft() + area.getTopLeft();
	Vector2i end = start + area.getSize();
	end.x = std::min(end.x, viewPort.getBottomRight().x);
	end.y = std::min(end.y, viewPort.getBottomRight().y);
	return RenderContext(painter, camera, renderTarget, Rect4i(start, end));
}
*/
