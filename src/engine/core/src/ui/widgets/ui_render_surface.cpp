#include "halley/ui/widgets/ui_render_surface.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target_texture.h"

using namespace Halley;

UIRenderSurface::UIRenderSurface(String id, Vector2f minSize, std::optional<UISizer> sizer, const HalleyAPI& api, Resources& resources, const String& materialName, RenderSurfaceOptions options)
	: UIWidget(std::move(id), Vector2f::max(Vector2f(1, 1), minSize), std::move(sizer))
	, renderSurface(std::make_unique<RenderSurface>(*api.video, resources, materialName, options))
	, colour(1, 1, 1, 1)
	, scale(1, 1)
{
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

// Update thread
void UIRenderSurface::drawChildren(UIPainter& origPainter) const
{
	if (!isRendering()) {
		UIWidget::drawChildren(origPainter);
		return;
	}

	RenderParams params;
	params.spritePainter = std::make_unique<SpritePainter>();
	params.pos = getPosition();
	params.size = innerSize + innerSize % Vector2f(2, 2);
	params.colour = colour;
	params.scale = scale;
	params.mask = origPainter.getMask();

	auto clip = origPainter.getClip();
	if (clip) {
		clip = (*clip - params.pos) / scale + params.pos;
	}

	if (params.size.x > 0.1f && params.size.y > 0.1f) {
		params.spritePainter->startFrame(true); // force copy is only needed in multi-threaded rendering, but no way of knowing that from here
		auto painter = UIPainter(*params.spritePainter, origPainter.getMask(), 0).withClip(clip);
		UIWidget::drawChildren(painter);

		// Check sprite bounds and enlarge border if needed
		if (const auto tryBounds = params.spritePainter->getBounds()) {
			auto spriteBounds = *tryBounds;
			origPainter.addBounds(spriteBounds);

			auto myRect = Rect4f(params.pos, params.pos + params.size);
			myRect.setSize(myRect.getSize() + myRect.getSize() % Vector2f(2, 2));

			if (clip) {
				spriteBounds = spriteBounds.intersection(*clip);
			}

			params.border.x = spriteBounds.getLeft() - myRect.getLeft();
			params.border.y = spriteBounds.getTop() - myRect.getTop();
			params.border.z = myRect.getRight() - spriteBounds.getRight();
			params.border.w = myRect.getBottom() - spriteBounds.getBottom();
		}

		bool success = paramsSync.write(std::move(params));
		if (!success) {
			Logger::logError("Error writing UIRenderSurface::paramsSync");
		}
	}
}

// Update thread
void UIRenderSurface::collectWidgetsForRendering(size_t curRootIdx, Vector<std::pair<std::shared_ptr<UIWidget>, size_t>>& dst, Vector<std::shared_ptr<UIWidget>>& dstRoots)
{
	if (isRendering()) {
		auto me = shared_from_this();
		dstRoots.push_back(me);

		for (auto& c: getChildren()) {
			c->collectWidgetsForRendering(curRootIdx + 1, dst, dstRoots);
		}
		dst.emplace_back(me, curRootIdx);
	} else {
		UIWidget::collectWidgetsForRendering(curRootIdx, dst, dstRoots);
	}
}

// Update thread
bool UIRenderSurface::hasRender() const
{
	return true;
}

// Render thread
void UIRenderSurface::onPreRender()
{
	renderParams = paramsSync.read();
}

// Render thread
RenderContext UIRenderSurface::getRenderContextForChildren(RenderContext& rc)
{
    if (!renderParams) {
        return rc;
    }

	auto& cam = renderParams->camera; // Camera cannot be a local variable, returned RC will take a reference to it

	cam = rc.getCamera();
	cam.setScale(cam.getScale().xy() * renderParams->scale);
	return rc.with(cam);
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
	cam.setPosition(renderParams->pos + (renderParams->border.xy() - renderParams->border.zw()) / 2 + renderParams->size / 2);
	cam.setScale(scale);

	const auto borderSize = renderParams->border.xy() + renderParams->border.zw();
	renderSurface->setSize(Vector2i((renderParams->size - borderSize) * scale));

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

		auto sprite = renderSurface->getSurfaceSprite().clone()
			.setPosition(renderParams->pos + renderParams->border.xy() * renderParams->scale)
			.setColour(renderParams->colour)
			.setScale(Vector2f(1.0f, 1.0f) / origScale);

		sprite.draw(painter);

		renderParams = {};
	}
}

void UIRenderSurface::setColour(Colour4f col)
{
	colour = col;
}

Colour4f UIRenderSurface::getColour() const
{
	return colour;
}

void UIRenderSurface::setScale(Vector2f scale)
{
	this->scale = scale;
	markAsNeedingLayout();
}

Vector2f UIRenderSurface::getLayoutMinimumSize(bool force) const
{
	const auto sz = UIWidget::getLayoutMinimumSize(force);
	return isRendering() ? (sz * scale).ceil() : sz;
}

Vector2f UIRenderSurface::getLayoutSize(Vector2f size) const
{
	innerSize = isRendering() ? (size / scale).ceil() : size;
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

void UIRenderSurface::setAutoBypass(bool autoBypass)
{
	this->autoBypass = autoBypass;
}

bool UIRenderSurface::isRendering() const
{
	return isEnabled() && !bypass;
}

void UIRenderSurface::fade(Colour4f from, Colour4f to, Time time, Time delay)
{
	fading = Fade{ from, to, time, -delay };
}

void UIRenderSurface::fade(float from, float to, Time time, Time delay)
{
	fade(Colour4f(1, 1, 1, from), Colour4f(1, 1, 1, to), time, delay);
}

void UIRenderSurface::update(Time t, bool moved)
{
	if (fading) {
		fading->curTime += t;
		const auto v = static_cast<float>(clamp(fading->curTime / fading->length, 0.0, 1.0));
		setColour(lerp(fading->from, fading->to, v));
		if (fading->curTime >= fading->length) {
			setColour(fading->to);
			fading = {};
		}
	}

	if (autoBypass) {
		bypass = Colour4c(colour) == Colour4c(255, 255, 255, 255) && std::abs(scale.x - 1.0f) < 0.00001f && std::abs(scale.y - 1.0f) < 0.00001f;
	}
}

bool UIRenderSurface::ignoreClip() const
{
	return true;
}
