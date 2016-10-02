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
	Expects(material);
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	painter.drawSprites(material, 1, &vertexAttrib);
}

void Sprite::drawSliced(Painter& painter, Vector4f slices) const
{
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	painter.drawSlicedSprite(material, scale, slices, &vertexAttrib);
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

void Sprite::computeSize()
{
	vertexAttrib.size = scale * size;
	if (flip) {
		vertexAttrib.size.x *= -1;
	}
}

bool Sprite::isInView(Rect4f v) const
{
	// This is a coarse test; will give a few false positives
	Vector2f pos = vertexAttrib.pos;
	Vector2f sz = vertexAttrib.size * 1.4142136f; // sqrt(2)
	Rect4f rect(pos - sz, pos + sz); // Could use offset here, but that would also need to take rotation into account

	return rect.overlaps(v);
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

Sprite& Sprite::setImage(std::shared_ptr<Texture> image, std::shared_ptr<MaterialDefinition> materialDefinition)
{
	Expects(image);
	Expects(materialDefinition);

	auto mat = std::make_shared<Material>(materialDefinition);
	(*mat)["tex0"] = image;
	setMaterial(mat);
	setSize(Vector2f(image->getSize()));
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
	vertexAttrib.rotation.x = v.getRadians();
	return *this;
}

Sprite& Sprite::setColour(Colour4f v)
{
	vertexAttrib.colour = v;
	return *this;
}

Sprite& Sprite::setScale(Vector2f v)
{
	if (scale != v) {
		scale = v;
		computeSize();
	}
	return *this;
}

Sprite& Sprite::setScale(float scale)
{
	return setScale(Vector2f(scale, scale));
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

Sprite& Sprite::setSprite(const SpriteSheetEntry& entry)
{
	if (vertexAttrib.texRect != entry.coords) {
		setSize(entry.size);
		vertexAttrib.pivot = entry.pivot;
		vertexAttrib.texRect = entry.coords;
		vertexAttrib.rotation.y = entry.rotated ? 1.0f : 0.0f;
	}
	return *this;
}

Sprite Sprite::clone() const
{
	return *this;
}
