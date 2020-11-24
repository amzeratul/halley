#include "widgets/ui_multi_image.h"
using namespace Halley;

UIMultiImage::UIMultiImage(const String& id, Vector2f size, std::vector<Sprite> sprites, std::vector<Vector2f> offsets)
	: UIWidget(id, size)
	, sprites(std::move(sprites))
	, offsets(std::move(offsets))
{
	if(sprites.size() != offsets.size()) {
		offsets.resize(sprites.size());
	}
}

Sprite& UIMultiImage::getSprite(const int index)
{
	return sprites.at(index);
}

const Sprite& UIMultiImage::getSprite(const int index) const
{
	return sprites.at(index);
}

const std::vector<Sprite>& UIMultiImage::getSprites() const
{
	return sprites;
}

void UIMultiImage::setSprite(const int index, Sprite sprite)
{
	sprites[index] = std::move(sprite);
	dirty = true;
}

void UIMultiImage::setSprites(std::vector<Sprite> sprites)
{
	this->sprites = std::move(sprites);
	if (this->sprites.size() != offsets.size()) {
		offsets.resize(this->sprites.size());
	}
	dirty = true;
}

Vector2f UIMultiImage::getOffset(const int index) const
{
	return offsets.at(index);
}

const std::vector<Vector2f>& UIMultiImage::getOffsets() const
{
	return offsets;
}

void UIMultiImage::setOffset(const int index, Vector2f offset)
{
	offsets[index] = offset;
	dirty = true;
}

void UIMultiImage::setOffsets(std::vector<Vector2f> offsets)
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
