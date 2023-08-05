#include "halley/ui/widgets/ui_render_surface.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target_texture.h"

using namespace Halley;

UIRenderSurface::UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer, const HalleyAPI& api, Resources& resources, const String& materialName, RenderSurfaceOptions options)
	: UIWidget(std::move(id), minSize, std::move(sizer))
	, spritePainter(std::make_unique<SpritePainter>())
	, renderSurface(std::make_unique<RenderSurface>(*api.video, resources, materialName, options))
	, colour(1, 1, 1, 1)
	, scale(1, 1)
{
}

// Update thread
void UIRenderSurface::update(Time t, bool moved)
{
	params.pos = getPosition();
	params.size = childrenMinSize;
	params.colour = colour;
	params.scale = scale;
}

// Update thread
void UIRenderSurface::drawChildren(UIPainter& origPainter) const
{
	if (isEnabled()) {
		// TODO: sync threads
		spritePainter->start(true); // force copy is only needed in multi-threaded rendering, but no way of knowing that from here
		params.mask = origPainter.getMask();
		auto painter = UIPainter(*spritePainter, origPainter.getMask(), 0);
		UIWidget::drawChildren(painter);
	} else {
		UIWidget::drawChildren(origPainter);
	}
}

// Update thread
void UIRenderSurface::draw(UIPainter& painter) const
{
	if (!isEnabled()) {
		return;
	}

	auto sharedThis = shared_from_this();
	painter.draw([sharedThis] (Painter& painter)
	{
		std::dynamic_pointer_cast<const UIRenderSurface>(sharedThis)->drawOnPainter(painter);
	});
}

// Render thread
void UIRenderSurface::render(RenderContext& rc) const
{
	if (!isEnabled() || params.size.x <= 0.1f || params.size.y <= 0.1f) {
		return;
	}
	Camera cam = rc.getCamera();
	cam.setPosition((params.pos + params.size / 2).round());
	cam.setScale(params.scale);

	renderSurface->setSize(Vector2i(params.size * params.scale));

	rc.with(renderSurface->getRenderTarget()).with(cam).bind([=](Painter& painter)
	{
		painter.clear(Colour4f(0, 0, 0, 0));
		spritePainter->draw(params.mask, painter);
	});
}

void UIRenderSurface::setColour(Colour4f col)
{
	colour = col;
}

void UIRenderSurface::setScale(Vector2f scale)
{
	this->scale = scale;
}

Vector2f UIRenderSurface::getLayoutMinimumSize(bool force) const
{
	childrenMinSize = UIWidget::getLayoutMinimumSize(force);
	return isEnabled() ? childrenMinSize * scale : childrenMinSize;
}

Vector2f UIRenderSurface::getLayoutSize(Vector2f size) const
{
	return childrenMinSize;
}

void UIRenderSurface::onPreNotifySetRect(IUIElementListener& listener)
{
	if (isEnabled()) {
		auto m = Matrix4f::makeIdentity();
		m.translate(getPosition());
		m.scale(scale);
		m.translate(-getPosition());
		listener.applyTransform(m);
	}
}

std::optional<Vector2f> UIRenderSurface::transformToChildSpace(Vector2f pos) const
{
	if (isEnabled()) {
		const auto p0 = getPosition();
		return (pos - p0) / scale + p0;
	} else {
		return pos;
	}
}

// Render thread
void UIRenderSurface::drawOnPainter(Painter& painter) const
{
	if (renderSurface->isReady()) {
		renderSurface->getSurfaceSprite().clone()
			.setPosition(params.pos)
			.setColour(params.colour)
			.draw(painter);
	}
}
