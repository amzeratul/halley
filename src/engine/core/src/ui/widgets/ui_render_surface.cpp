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
void UIRenderSurface::drawChildren(UIPainter& origPainter) const
{
	if (isRendering()) {
		RenderParams params;
		params.spritePainter = std::make_unique<SpritePainter>();
		params.pos = getPosition();
		params.size = innerSize + innerSize % Vector2f(2, 2);
		params.colour = colour;
		params.scale = scale;
		params.mask = origPainter.getMask();

		if (params.size.x > 0.1f && params.size.y > 0.1f) {
			params.spritePainter->start(true); // force copy is only needed in multi-threaded rendering, but no way of knowing that from here
			auto painter = UIPainter(*params.spritePainter, origPainter.getMask(), 0);
			UIWidget::drawChildren(painter);

			// Check sprite bounds and enlarge border if needed
			const auto tryBounds = params.spritePainter->getBounds();
			if (tryBounds) {
				const auto bounds = *tryBounds;
				auto myRect = Rect4f(params.pos, params.pos + params.size);
				myRect.setSize(myRect.getSize() + myRect.getSize() % Vector2f(2, 2));
				params.border.x = std::abs(myRect.getLeft() - bounds.getLeft());
				params.border.y = std::abs(myRect.getTop() - bounds.getTop());
				params.border.z = std::abs(bounds.getRight() - myRect.getRight());
				params.border.w = std::abs(bounds.getBottom() - myRect.getBottom());
			}

			paramsSync.write(std::move(params));
		}
	} else {
		UIWidget::drawChildren(origPainter);
	}
}

// Update thread
void UIRenderSurface::draw(UIPainter& painter) const
{
	if (!isRendering()) {
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
	if (!renderParams) {
		return;
	}

	Camera cam = rc.getCamera();
	origScale = cam.getScale().xy();
	const auto scale = renderParams->scale * origScale;
	cam.setPosition(renderParams->pos - renderParams->border.xy() / 2 + renderParams->border.zw() / 2 + renderParams->size / 2);
	cam.setScale(scale);

	const auto borderSize = renderParams->border.xy() + renderParams->border.zw();
	renderSurface->setSize(Vector2i((renderParams->size + borderSize) * scale));

	rc.with(renderSurface->getRenderTarget()).with(cam).bind([&](Painter& painter)
	{
		painter.clear(Colour4f(0, 0, 0, 0));
		renderParams->spritePainter->draw(renderParams->mask, painter);
	});
}

// Render thread
void UIRenderSurface::renderChildren(RenderContext& rc) const
{
	// renderChildren runs before render, so we acquire renderParams here
	renderParams = paramsSync.read();

	if (renderParams) {
		auto cam = rc.getCamera();
		cam.setScale(cam.getScale().xy() * renderParams->scale);
		auto rc2 = rc.with(cam);
		UIWidget::renderChildren(rc2);
	} else {
		UIWidget::renderChildren(rc);
	}
}

// Render thread
void UIRenderSurface::drawOnPainter(Painter& painter) const
{
	if (renderParams) {
		assert(renderSurface->isReady());

		const auto scale = renderParams->scale * origScale;
		
		renderSurface->getSurfaceSprite().clone()
			.setPosition(renderParams->pos - renderParams->border.xy() * scale)
			.setColour(renderParams->colour)
			.setScale(Vector2f(1.0f, 1.0f) / origScale)
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
	markAsNeedingLayout();
}

Vector2f UIRenderSurface::getLayoutMinimumSize(bool force) const
{
	const auto sz = UIWidget::getLayoutMinimumSize(force);
	return isRendering() ? sz * scale : sz;
}

Vector2f UIRenderSurface::getLayoutSize(Vector2f size) const
{
	innerSize = isRendering() ? (size / scale).round() : size;
	return innerSize;
}

void UIRenderSurface::onPreNotifySetRect(IUIElementListener& listener)
{
	if (isRendering()) {
		auto m = Matrix4f::makeIdentity();
		m.translate(getPosition());
		m.scale(scale);
		m.translate(-getPosition());
		listener.applyTransform(m);
	}
}

std::optional<Vector2f> UIRenderSurface::transformToChildSpace(Vector2f pos) const
{
	if (isRendering()) {
		const auto p0 = getPosition();
		return (pos - p0) / scale + p0;
	} else {
		return pos;
	}
}

void UIRenderSurface::setBypass(bool bypass)
{
	this->bypass = bypass;
}

bool UIRenderSurface::isRendering() const
{
	return isEnabled() && !bypass;
}
