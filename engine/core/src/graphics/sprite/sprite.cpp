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
	
	painter.drawSprites(material, 1, &vertexAttrib);
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
	painter.drawSlicedSprite(material, vertexAttrib.scale, slices, &vertexAttrib);
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
	// This is a coarse test; will give a few false positives
	Vector2f pos = vertexAttrib.pos;
	Vector2f sz = vertexAttrib.size * 1.4142136f; // sqrt(2)
	return Rect4f(pos - sz, pos + sz); // Could use offset here, but that would also need to take rotation into account
}

bool Sprite::isInView(Rect4f v) const
{
	return getAABB().overlaps(v);
}

Sprite& Sprite::setMaterial(Resources& resources, String materialName)
{
	if (materialName == "") {
		materialName = "sprite";
	}
	setMaterial(std::make_shared<Material>(resources.get<MaterialDefinition>(materialName)));
	return *this;
}

Sprite& Sprite::setMaterial(std::shared_ptr<Material> m)
{
	Expects(m);
	material = m;

	auto& mainTex = material->getMainTexture();
	if (mainTex) {
		setImageData(*mainTex);
	}

	return *this;
}

Sprite& Sprite::setImage(Resources& resources, String imageName, String materialName)
{
	if (materialName == "") {
		materialName = "sprite";
	}
	setImage(resources.get<Texture>(imageName), resources.get<MaterialDefinition>(materialName));
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

Sprite& Sprite::setImageData(const Texture& image)
{
	setSize(Vector2f(image.getSize()));
	setTexRect(Rect4f(0, 0, 1, 1));
	return *this;
}

Vector2f Sprite::getPosition() const
{
	return vertexAttrib.pos;
}

Sprite& Sprite::setPos(Vector2f v)
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

Sprite& Sprite::setSprite(Resources& resources, String spriteSheetName, String imageName, String materialName)
{
	if (materialName == "") {
		materialName = "sprite";
	}
	auto spriteSheet = resources.get<SpriteSheet>(spriteSheetName);
	setImage(spriteSheet->getTexture(), resources.get<MaterialDefinition>(materialName));
	setSprite(*spriteSheet, imageName);
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheet& sheet, String name)
{
	Expects(&sheet != nullptr);

	setSprite(sheet.getSprite(name));
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheetEntry& entry, bool applyPivot)
{
	if (vertexAttrib.texRect != entry.coords) {
		setSize(entry.size);
		if (applyPivot) {
			vertexAttrib.pivot = entry.pivot;
		}
		vertexAttrib.texRect = entry.coords;
		vertexAttrib.textureRotation = entry.rotated ? 1.0f : 0.0f;
	}
	return *this;
}

Sprite& Sprite::setSliced(Vector4s s)
{
	slices = s;
	sliced = true;
	return *this;
}

Sprite& Sprite::setSlicedFromMaterial()
{
	Expects (material);
	Expects (material->getMainTexture());
	return setSliced(material->getMainTexture()->getSlices());
}

Sprite& Sprite::setNormal()
{
	sliced = false;
	return *this;
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
