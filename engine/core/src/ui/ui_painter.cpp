#include "ui/ui_painter.h"
#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/text/text_renderer.h"
using namespace Halley;

UIPainter::UIPainter(SpritePainter& painter, int mask, int layer)
	: painter(painter)
	, mask(mask)
	, layer(layer)
	, n(0)
{
}

UIPainter UIPainter::clone()
{
	auto result = UIPainter(painter, mask, layer);
	result.parent = this;
	return result;
}

UIPainter UIPainter::withAdjustedLayer(int delta)
{
	auto result = clone();
	result.layer += delta;
	return result;
}

UIPainter UIPainter::withClip(Rect4f newClip)
{
	auto result = clone();
	if (clip) {
		result.clip = clip.get().intersection(newClip);
	} else {
		result.clip = newClip;
	}
	return result;
}

float UIPainter::getCurrentPriority()
{
	if (parent) {
		return parent->getCurrentPriority();
	} else {
		return float(n++);
	}
}

void UIPainter::draw(const Sprite& sprite)
{
	if (clip) {
		auto targetClip = clip.get() - sprite.getPosition();
		if (sprite.getClip()) {
			targetClip = sprite.getClip().get().intersection(targetClip);
		}

		auto onScreen = sprite.getAABB().intersection(targetClip + sprite.getPosition());
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(sprite.clone().setClip(targetClip), mask, layer, getCurrentPriority());
		}
	} else {
		painter.add(sprite, mask, layer, getCurrentPriority());
	}
}

void UIPainter::draw(const TextRenderer& text)
{
	if (clip) {
		auto targetClip = clip.get() - text.getPosition();
		if (text.getClip()) {
			targetClip = text.getClip().get().intersection(targetClip);
		}
		
		auto onScreen = Rect4f(Vector2f(), text.getExtents()).intersection(targetClip);
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(text.clone().setClip(clip.get() - text.getPosition()), mask, layer, getCurrentPriority());
		}
	} else {
		painter.add(text, mask, layer, getCurrentPriority());
	}
}
