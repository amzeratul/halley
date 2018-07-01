#include <cstring>
#include "graphics/sprite/sprite.h"
#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/texture.h"
#include "resources/resources.h"
#include <gsl/gsl_assert>

using namespace Halley;

Sprite::Sprite()
{
	setScale(Vector2f(1, 1));
	setColour(Colour4f(1, 1, 1, 1));
}

void Sprite::draw(Painter& painter) const
{
	if (sliced) {
		drawSliced(painter, slices);
	} else {
		drawNormal(painter);
	}
}

void Sprite::drawNormal(Painter& painter) const
{
	Expects(material);
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	
	if (clip) {
		painter.setRelativeClip(clip.get() + vertexAttrib.pos);
	}
	painter.drawSprites(material, 1, &vertexAttrib);
	if (clip) {
		painter.setClip();
	}
}

void Sprite::drawSliced(Painter& painter) const
{
	drawSliced(painter, slices);
}

void Sprite::drawSliced(Painter& painter, Vector4s slicesPixel) const
{
	Expects(material);
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	
	Vector4f slices(slicesPixel);
	slices.x /= size.x;
	slices.y /= size.y;
	slices.z /= size.x;
	slices.w /= size.y;

	if (clip) {
		painter.setRelativeClip(clip.get() + vertexAttrib.pos);
	}
	painter.drawSlicedSprite(material, vertexAttrib.scale, slices, &vertexAttrib);
	if (clip) {
		painter.setClip();
	}
}

void Sprite::draw(const Sprite* sprites, size_t n, Painter& painter) // static
{
	if (n == 0) {
		return;
	}

	auto& material = sprites[0].material;
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

	size_t spriteSize = sizeof(SpriteVertexAttrib);
	Vector<char> vertices(n * spriteSize);

	for (size_t i = 0; i < n; i++) {
		auto& sprite = sprites[i];
		Expects(sprite.material == material);
		memcpy(&vertices[i * spriteSize], &sprite.vertexAttrib, spriteSize);
	}

	painter.drawSprites(material, n, vertices.data());
}

Rect4f Sprite::getAABB() const
{
	Vector2f pos = vertexAttrib.pos;
	if (std::abs(vertexAttrib.rotation) < 0.0001f) {
		// No rotation, give exact bounding box
		Vector2f sz = getScaledSize();
		return Rect4f(pos - sz * vertexAttrib.pivot, pos + sz * (Vector2f(1, 1) - vertexAttrib.pivot));
	} else {
		// This is a coarse test; will give a few false positives
		Vector2f sz = getScaledSize() * 1.4142136f; // sqrt(2)
		return Rect4f(pos - sz, pos + sz); // Could use offset here, but that would also need to take rotation into account
	}
}

bool Sprite::isInView(Rect4f v) const
{
	return isVisible() && getAABB().overlaps(v);
}

Vector2f Sprite::getScaledSize() const
{
	return vertexAttrib.scale * vertexAttrib.size;
}

Vector2f Sprite::getPosition() const
{
	return vertexAttrib.pos;
}

Colour4f Sprite::getColour() const
{
	return vertexAttrib.colour;
}

Sprite& Sprite::setPos(Vector2f v)
{
	vertexAttrib.pos = v;
	return *this;
}

Sprite& Sprite::setPosition(Vector2f v)
{
	vertexAttrib.pos = v;
	return *this;
}

Sprite& Sprite::setRotation(Angle1f v)
{
	vertexAttrib.rotation = v.getRadians();
	return *this;
}

Sprite& Sprite::setColour(Colour4f v)
{
	vertexAttrib.colour = v;
	return *this;
}

Sprite& Sprite::setScale(Vector2f v)
{
	vertexAttrib.scale = v;
	return *this;
}

Sprite& Sprite::setScale(float scale)
{
	return setScale(Vector2f(scale, scale));
}

Sprite& Sprite::scaleTo(Vector2f newSize)
{
	return setScale(newSize / size);
}

Sprite& Sprite::setFlip(bool v)
{
	if (flip != v) {
		flip = v;
		computeSize();
	}
	return *this;
}

bool Sprite::isFlipped() const
{
	return flip;
}

Sprite& Sprite::setPivot(Vector2f v)
{
	vertexAttrib.pivot = v;
	return *this;
}

Sprite& Sprite::setAbsolutePivot(Vector2f v)
{
	vertexAttrib.pivot = v / size;
	return *this;
}

Vector2f Sprite::getPivot() const
{
	return vertexAttrib.pivot;
}

Vector2f Sprite::getAbsolutePivot() const
{
	return vertexAttrib.pivot * size;
}

Sprite& Sprite::setSize(Vector2f v)
{
	if (size != v) {
		size = v;
		computeSize();
	}
	return *this;
}

Sprite& Sprite::setTexRect(Rect4f v)
{
	vertexAttrib.texRect = v;
	return *this;
}

Sprite& Sprite::setMaterial(Resources& resources, String materialName)
{
	if (materialName == "") {
		materialName = "Halley/Sprite";
	}
	setMaterial(std::make_shared<Material>(resources.get<MaterialDefinition>(materialName)));
	return *this;
}

Sprite& Sprite::setMaterial(std::shared_ptr<Material> m)
{
	bool hadMaterial = static_cast<bool>(material);

	Expects(m);
	material = m;

	if (!hadMaterial && !material->getTextures().empty()) {
		setImageData(*material->getTextures()[0]);
	}

	return *this;
}

Sprite& Sprite::setImageData(const Texture& image)
{
	setSize(Vector2f(image.getSize()));
	setTexRect(Rect4f(0, 0, 1, 1));
	return *this;
}

Sprite& Sprite::setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> materialDefinition)
{
	Expects(image);
	Expects(materialDefinition);

	auto mat = std::make_shared<Material>(materialDefinition);
	mat->set("tex0", image);
	setMaterial(mat);
	return *this;
}

Sprite& Sprite::setImage(Resources& resources, String imageName, String materialName)
{
	/*
	Expects (!imageName.isEmpty());
	if (materialName == "") {
		materialName = "Halley/Sprite";
	}
	setImage(resources.get<Texture>(imageName), resources.get<MaterialDefinition>(materialName));
	return *this;
	*/

	Expects (!imageName.isEmpty());
	if (materialName == "") {
		materialName = "Halley/Sprite";
	}
	const auto sprite = resources.get<SpriteResource>(imageName);
	const auto spriteSheet = sprite->getSpriteSheet();
	setImage(spriteSheet->getTexture(), resources.get<MaterialDefinition>(materialName));
	setSprite(sprite->getSprite());
	return *this;
}

Sprite& Sprite::setSprite(Resources& resources, String spriteSheetName, String imageName, String materialName)
{
	Expects (!spriteSheetName.isEmpty());
	Expects (!imageName.isEmpty());

	if (materialName == "") {
		materialName = "Halley/Sprite";
	}
	auto spriteSheet = resources.get<SpriteSheet>(spriteSheetName);
	setImage(spriteSheet->getTexture(), resources.get<MaterialDefinition>(materialName));
	setSprite(*spriteSheet, imageName);
	return *this;
}

Sprite& Sprite::setSprite(const SpriteResource& sprite, bool applyPivot)
{
	setSprite(sprite.getSprite(), applyPivot);
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheet& sheet, String name, bool applyPivot)
{
	Expects (!name.isEmpty());
	setSprite(sheet.getSprite(name), applyPivot);
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheetEntry& entry, bool applyPivot)
{
	outerBorder = entry.trimBorder;
	slices = entry.slices;
	sliced = slices.x != 0 || slices.y != 0 || slices.z != 0 || slices.w != 0;
	setSize(entry.size);
	if (applyPivot) {
		vertexAttrib.pivot = entry.pivot;
	}
	vertexAttrib.texRect = entry.coords;
	vertexAttrib.textureRotation = entry.rotated ? 1.0f : 0.0f;
	return *this;
}

Sprite& Sprite::setSliced(Vector4s s)
{
	slices = s;
	sliced = true;
	return *this;
}

Sprite& Sprite::setNotSliced()
{
	sliced = false;
	return *this;
}

Sprite& Sprite::setVisible(bool v)
{
	visible = v;
	return *this;
}

bool Sprite::isVisible() const
{
	return visible;
}

Sprite& Sprite::setClip(Rect4f c)
{
	clip = c;
	return *this;
}

Sprite& Sprite::setClip()
{
	clip.reset();
	return *this;
}

Maybe<Rect4f> Sprite::getClip() const
{
	return clip;
}

Sprite Sprite::clone() const
{
	return *this;
}

void Sprite::computeSize()
{
	vertexAttrib.size = size;
	if (flip) {
		vertexAttrib.size.x *= -1;
	}
}

Vector2f Sprite::getSize() const
{
	return size;
}

Vector2f Sprite::getRawSize() const
{
	return vertexAttrib.size;
}

Vector2f Sprite::getOriginalSize() const
{
	return getRawSize() + Vector2f(Vector2i(outerBorder.x + outerBorder.z, outerBorder.y + outerBorder.z));
}

Vector4s Sprite::getOuterBorder() const
{
	return outerBorder;
}

Sprite& Sprite::setOuterBorder(Vector4s border)
{
	outerBorder = border;
	return *this;
}

Vector2f Sprite::getScale() const
{
	return vertexAttrib.scale;
}
