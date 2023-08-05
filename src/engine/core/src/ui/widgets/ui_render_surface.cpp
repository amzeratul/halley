#include "halley/ui/widgets/ui_render_surface.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target_texture.h"

using namespace Halley;

UIRenderSurface::UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer, const HalleyAPI& api, Resources& resources, const String& materialName, RenderSurfaceOptions options)
	: UIWidget(std::move(id), minSize, std::move(sizer))
	, renderSurface(std::make_unique<RenderSurface>(*api.video, resources, materialName, options))
	, colour(1, 1, 1, 1)
	, scale(1, 1)
{
}

// Update thread
void UIRenderSurface::update(Time t, bool moved)
{
}

// Update thread
void UIRenderSurface::drawChildren(UIPainter& origPainter) const
{
	if (isEnabled()) {
		RenderParams params;
		params.spritePainter = std::make_unique<SpritePainter>();
		params.pos = getPosition();
		params.size = childrenMinSize;
		params.colour = colour;
		params.scale = scale;
		params.mask = origPainter.getMask();

		if (params.size.x > 0.1f && params.size.y > 0.1f) {
			params.spritePainter->start(true); // force copy is only needed in multi-threaded rendering, but no way of knowing that from here
			auto painter = UIPainter(*params.spritePainter, origPainter.getMask(), 0);
			UIWidget::drawChildren(painter);

			paramsSync.write(std::move(params));
		}
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
	renderParams = paramsSync.read();
	if (!renderParams) {
		return;
	}

	Camera cam = rc.getCamera();
	cam.setPosition((renderParams->pos + renderParams->size / 2).round());
	cam.setScale(renderParams->scale);

	renderSurface->setSize(Vector2i(renderParams->size * renderParams->scale));

	rc.with(renderSurface->getRenderTarget()).with(cam).bind([&](Painter& painter)
	{
		painter.clear(Colour4f(0, 0, 0, 0));
		renderParams->spritePainter->draw(renderParams->mask, painter);
	});
}

// Render thread
void UIRenderSurface::drawOnPainter(Painter& painter) const
{
	if (renderParams) {
		assert(renderSurface->isReady());

		renderSurface->getSurfaceSprite().clone()
			.setPosition(renderParams->pos)
			.setColour(renderParams->colour)
			.draw(painter);

		renderParams = {};
	}
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
