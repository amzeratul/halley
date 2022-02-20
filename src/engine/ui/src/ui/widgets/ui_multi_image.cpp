#include "widgets/ui_multi_image.h"
using namespace Halley;

UIMultiImage::UIMultiImage(const String& id, Vector2f size, Vector<Sprite> sprites, Vector<Vector2f> offsets)
	: UIWidget(id, size)
	, sprites(std::move(sprites))
	, offsets(std::move(offsets))
{
	if(this->sprites.size() != this->offsets.size()) {
		this->offsets.resize(this->sprites.size());
	}
}

Sprite& UIMultiImage::getSprite(size_t index)
{
	return sprites.at(index);
}

const Sprite& UIMultiImage::getSprite(size_t index) const
{
	return sprites.at(index);
}

const Vector<Sprite>& UIMultiImage::getSprites() const
{
	return sprites;
}

void UIMultiImage::setSprite(size_t index, Sprite sprite)
{
	sprites[index] = std::move(sprite);
	dirty = true;
}

void UIMultiImage::setSprites(Vector<Sprite> sprites)
{
	this->sprites = std::move(sprites);
	if (this->sprites.size() != offsets.size()) {
		offsets.resize(this->sprites.size());
	}
	dirty = true;
}

Vector2f UIMultiImage::getOffset(size_t index) const
{
	return offsets.at(index);
}

const Vector<Vector2f>& UIMultiImage::getOffsets() const
{
	return offsets;
}

void UIMultiImage::setOffset(size_t index, Vector2f offset)
{
	this->offsets[index] = offset;
	dirty = true;
}

void UIMultiImage::setOffsets(Vector<Vector2f> offsets)
{
	this->offsets = std::move(offsets);
	if (this->sprites.size() != offsets.size()) {
		offsets.resize(this->sprites.size());
	}
	dirty = true;
}

void UIMultiImage::update(Time t, bool moved)
{
	if (moved || dirty) {
		const Vector2f basePos = getPosition() + getSize() * 0.5f;

		for (int i = 0; i < sprites.size(); i++) {
			sprites[i].setPos(basePos + offsets[i]);
		}
		dirty = false;
	}
}

void UIMultiImage::draw(UIPainter& painter) const
{
	for(const auto& sprite : sprites) {
		painter.draw(sprite);
	}
}
