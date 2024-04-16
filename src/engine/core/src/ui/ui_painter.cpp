#include "halley/ui/ui_painter.h"
#include "halley/graphics/sprite/sprite_painter.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/text_renderer.h"
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
	result.currentPriority = currentPriority;
	result.colourMultiplier = colourMultiplier;
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
	if (newClip && clip) {
		result.clip = clip->intersection(newClip.value());
	} else {
		result.clip = newClip;
	}
	return result;
}

UIPainter UIPainter::withNoClip() const
{
	auto result = clone();
	result.clip = {};
	return result;
}

UIPainter UIPainter::withAlpha(float alpha) const
{
	auto result = clone();
	auto col = colourMultiplier.value_or(Colour4f(1, 1, 1, 1));
	col.a *= alpha;
	result.colourMultiplier = col;
	return result;
}

UIPainter UIPainter::withColour(Colour4f colour) const
{
	auto result = clone();
	result.colourMultiplier = colour * colourMultiplier.value_or(Colour4f(1, 1, 1, 1));
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
	if (colourMultiplier) {
		painter->add(applyColour(sprite), mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else if (forceCopy) {
		painter->addCopy(sprite, mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else {
		painter->add(sprite, mask, layer, getCurrentPriorityAndIncrement(), clip);
	}
}

void UIPainter::draw(const TextRenderer& text, bool forceCopy)
{
	if (colourMultiplier) {
		painter->add(applyColour(text), mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else if (forceCopy) {
		painter->addCopy(text, mask, layer, getCurrentPriorityAndIncrement(), clip);
	} else {
		painter->add(text, mask, layer, getCurrentPriorityAndIncrement(), clip);
	}
}

void UIPainter::draw(Sprite&& sprite)
{
	painter->add(applyColour(std::move(sprite)), mask, layer, getCurrentPriorityAndIncrement(), clip);
}

void UIPainter::draw(TextRenderer&& text)
{
	painter->add(applyColour(std::move(text)), mask, layer, getCurrentPriorityAndIncrement(), clip);
}

void UIPainter::draw(std::function<void(Painter&)> f)
{
	painter->add(std::move(f), mask, layer, getCurrentPriorityAndIncrement(), clip);
}

TextRenderer UIPainter::applyColour(TextRenderer text) const
{
	if (colourMultiplier) {
		const auto colMult = *colourMultiplier;
		const auto c = text.getColour();
		const auto co = text.getOutlineColour();
		const auto cs = text.getShadowColour();
		text.setColour(c * colMult);
		text.setOutlineColour(co * colMult);
		text.setShadowColour(cs * colMult);
	}
	return text;
}

Sprite UIPainter::applyColour(Sprite sprite) const
{
	if (colourMultiplier) {
		sprite.setColour(sprite.getColour() * *colourMultiplier);
	}
	return sprite;
}
