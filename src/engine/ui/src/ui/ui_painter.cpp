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

int UIPainter::getMask() const
{
	return mask;
}

float UIPainter::getCurrentPriorityAndIncrement() const
{
	if (rootPainter) {
		return rootPainter->getCurrentPriorityAndIncrement();
	} else {
		return static_cast<float>(currentPriority++);
	}
}

void UIPainter::draw(const Sprite& sprite, bool forceCopy)
{
	if (forceCopy) {
		painter->addCopy(sprite, mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else {
		painter->add(sprite, mask, layer, getCurrentPriorityAndIncrement(), clip);
	}
}

void UIPainter::draw(const TextRenderer& text, bool forceCopy)
{
	if (forceCopy) {
		painter->addCopy(text, mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else {
		painter->add(text, mask, layer, getCurrentPriorityAndIncrement(), clip);
	}
}

void UIPainter::draw(std::function<void(Painter&)> f)
{
	painter->add(std::move(f), mask, layer, getCurrentPriorityAndIncrement(), clip);
}
