#include <cstring>
#include "graphics/sprite/sprite.h"
#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/texture.h"
#include "resources/resources.h"

using namespace Halley;

Sprite::Sprite()
{
	vertices[0].vertPos = Vector2f(0, 0);
	vertices[1].vertPos = Vector2f(1, 0);
	vertices[2].vertPos = Vector2f(1, 1);
	vertices[3].vertPos = Vector2f(0, 1);
	setScale(Vector2f(1, 1));
	setColour(Colour4f(1, 1, 1, 1));
}

void Sprite::draw(Painter& painter) const
{
	update();

	assert(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	painter.drawQuads(material, 4, vertices.data());
}

void Sprite::draw(const Sprite* sprites, size_t n, Painter& painter)
{
	if (n == 0) {
		return;
	}

	auto& material = sprites[0].material;
	assert(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

	size_t spriteSize = 4 * sizeof(SpriteVertexAttrib);
	std::vector<char> vertices(n * spriteSize);

	for (size_t i = 0; i < n; i++) {
		auto& sprite = sprites[i];
		assert(sprite.material == material);
		sprite.update();
		memcpy(&vertices[i * spriteSize], &sprite.vertices[0], spriteSize);
	}

	painter.drawQuads(material, 4 * n, vertices.data());
}

void Sprite::update() const // Not really "const", but needs to be called from draw()
{
	if (dirty) {
		// Don't copy the last Vector2f (vertPos)
		constexpr size_t size = sizeof(SpriteVertexAttrib) - sizeof(Vector2f);
		memcpy(&vertices[1], &vertices[0], size);
		memcpy(&vertices[2], &vertices[0], size);
		memcpy(&vertices[3], &vertices[0], size);

		dirty = false;
	}
}

void Sprite::computeSize()
{
	vertices[0].size = scale * size;
	if (flip) {
		vertices[0].size.x *= -1;
	}
}

bool Sprite::isInView(Rect4f v) const
{
	// This is a coarse test; will give a few false positives
	Vector2f pos = vertices[0].pos;
	Vector2f sz = vertices[0].size * 1.4142136f; // sqrt(2)
	Rect4f rect(pos - sz, pos + sz); // Could use offset here, but that would also need to take rotation into account

	return rect.intersects(v);
}

Sprite& Sprite::setImage(Resources& resources, String imageName, String materialName)
{
	if (materialName == "") {
		materialName = "sprite.yaml";
	}
	setImage(resources.get<Texture>(imageName), resources.get<MaterialDefinition>(materialName));
	return *this;
}

Sprite& Sprite::setImage(std::shared_ptr<Texture> image, std::shared_ptr<MaterialDefinition> materialDefinition)
{
	auto mat = std::make_shared<Material>(materialDefinition);
	(*mat)["tex0"] = image;
	setMaterial(mat);
	setSize(Vector2f(image->getSize()));
	setTexRect(Rect4f(0, 0, 1, 1));
	return *this;
}

Vector2f Sprite::getPosition() const
{
	return vertices[0].pos;
}

Sprite& Sprite::setPos(Vector2f v)
{
	if (v != vertices[0].pos) {
		dirty = true;
		vertices[0].pos = v;
	}
	return *this;
}

Sprite& Sprite::setRotation(Angle1f v)
{
	if (v != vertices[0].rotation.x) {
		dirty = true;
		vertices[0].rotation.x = v.getRadians();
	}
	return *this;
}

Sprite& Sprite::setColour(Colour4f v)
{
	if (vertices[0].colour != v) {
		dirty = true;
		vertices[0].colour = v;
	}
	return *this;
}

Sprite& Sprite::setScale(Vector2f v)
{
	if (scale != v) {
		dirty = true;
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
		dirty = true;
		flip = v;
		computeSize();
	}
	return *this;
}

Sprite& Sprite::setPivot(Vector2f v)
{
	if (vertices[0].pivot != v) {
		dirty = true;
		vertices[0].pivot = v;
	}
	return *this;
}

Sprite& Sprite::setSize(Vector2f v)
{
	if (size != v) {
		dirty = true;
		size = v;
		computeSize();
	}
	return *this;
}

Sprite& Sprite::setTexRect(Rect4f v)
{
	if (vertices[0].texRect != v) {
		dirty = true;
		vertices[0].texRect = v;
	}
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheet& sheet, String name)
{
	setSprite(sheet.getSprite(name));
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheetEntry& entry)
{
	if (vertices[0].texRect != entry.coords) {
		setSize(entry.size);
		vertices[0].pivot = entry.pivot;
		vertices[0].texRect = entry.coords;
		vertices[0].rotation.y = entry.rotated ? 1.0f : 0.0f;
		dirty = true;
	}
	return *this;
}
