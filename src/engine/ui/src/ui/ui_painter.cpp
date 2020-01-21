#include "ui_painter.h"
#include "halley/core/graphics/sprite/sprite_painter.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
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
	result.clip = clip;
	return result;
}

UIPainter UIPainter::withAdjustedLayer(int delta)
{
	auto result = clone();
	result.layer += delta;
	return result;
}

UIPainter UIPainter::withClip(Maybe<Rect4f> newClip)
{
	auto result = clone();
	if (newClip) {
		if (clip) {
			result.clip = clip->intersection(newClip.value());
		} else {
			result.clip = newClip;
		}
	}
	return result;
}

UIPainter UIPainter::withMask(int mask)
{
	auto result = clone();
	result.mask = mask;
	return result;
}

Maybe<Rect4f> UIPainter::getClip() const
{
	return clip;
}

float UIPainter::getCurrentPriority()
{
	if (parent) {
		return parent->getCurrentPriority();
	} else {
		return float(n++);
	}
}

void UIPainter::draw(const Sprite& sprite, bool forceCopy)
{
	if (clip) {
		auto targetClip = clip.value() - sprite.getPosition();
		if (sprite.getClip()) {
			targetClip = sprite.getClip()->intersection(targetClip);
		}

		auto onScreen = sprite.getAABB().intersection(targetClip + sprite.getPosition());
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(sprite.clone().setClip(targetClip), mask, layer, getCurrentPriority());
		}
	} else {
		if (forceCopy) {
			painter.addCopy(sprite, mask, layer, getCurrentPriority());
		} else {
			painter.add(sprite, mask, layer, getCurrentPriority());
		}
	}
}

void UIPainter::draw(const TextRenderer& text, bool forceCopy)
{
	if (clip) {
		auto targetClip = clip.value() - text.getPosition();
		if (text.getClip()) {
			targetClip = text.getClip()->intersection(targetClip);
		}
		
		auto onScreen = Rect4f(Vector2f(), text.getExtents()).intersection(targetClip);
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(text.clone().setClip(clip.value() - text.getPosition()), mask, layer, getCurrentPriority());
		}
	} else {
		if (forceCopy) {
			painter.addCopy(text, mask, layer, getCurrentPriority());
		} else {
			painter.add(text, mask, layer, getCurrentPriority());
		}
	}
}
