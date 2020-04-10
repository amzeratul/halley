#include "ui_painter.h"
#include "halley/core/graphics/sprite/sprite_painter.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
using namespace Halley;

UIPainter::UIPainter(SpritePainter& painter, int mask, int layer)
	: painter(&painter)
	, mask(mask)
	, layer(layer)
{
}

UIPainter UIPainter::clone() const
{
	auto result = UIPainter(*painter, mask, layer);
	result.rootPainter = rootPainter ? rootPainter : this;
	result.clip = clip;
	return result;
}

UIPainter UIPainter::withAdjustedLayer(int delta) const
{
	auto result = clone();
	result.layer += delta;
	return result;
}

UIPainter UIPainter::withClip(std::optional<Rect4f> newClip) const
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

UIPainter UIPainter::withNoClip() const
{
	auto result = clone();
	result.clip = {};
	return result;
}

UIPainter UIPainter::withMask(int mask) const
{
	auto result = clone();
	result.mask = mask;
	return result;
}

std::optional<Rect4f> UIPainter::getClip() const
{
	return clip;
}

float UIPainter::getCurrentPriorityAndIncrement() const
{
	if (rootPainter) {
		return rootPainter->getCurrentPriorityAndIncrement();
	} else {
		return float(currentPriority++);
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
			painter->addCopy(sprite.clone().setClip(targetClip), mask, layer, getCurrentPriorityAndIncrement());
		}
	} else {
		if (forceCopy) {
			painter->addCopy(sprite, mask, layer, getCurrentPriorityAndIncrement());
		} else {
			painter->add(sprite, mask, layer, getCurrentPriorityAndIncrement());
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
			painter->addCopy(text.clone().setClip(clip.value() - text.getPosition()), mask, layer, getCurrentPriorityAndIncrement());
		}
	} else {
		if (forceCopy) {
			painter->addCopy(text, mask, layer, getCurrentPriorityAndIncrement());
		} else {
			painter->add(text, mask, layer, getCurrentPriorityAndIncrement());
		}
	}
}
