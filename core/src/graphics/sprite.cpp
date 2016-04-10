#include "sprite.h"
#include "painter.h"
#include "material.h"

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
	if (dirty) {
		update();
		dirty = false;
	}

	assert(material->getVertexStride() == sizeof(SpriteVertexAttrib));
	painter.drawVertices(*material, 4, vertices.data());
}

void Sprite::update() const // Not really "const", but needs to be called from draw()
{
	// Don't copy the last Vector2f (vertPos)
	constexpr size_t size = sizeof(SpriteVertexAttrib) - sizeof(Vector2f);
	memcpy(&vertices[1], &vertices[0], size);
	memcpy(&vertices[2], &vertices[0], size);
	memcpy(&vertices[3], &vertices[0], size);
}

bool Sprite::isInView(Rect4f v) const
{
	// TODO
	return true;
}

void Sprite::setPos(Vector2f v)
{
	dirty = true;
	vertices[0].pos = v;
}

void Sprite::setRotation(Angle1f v)
{
	dirty = true;
	vertices[0].rotation.x = v.getRadians();
}

void Sprite::setColour(Colour4f v)
{
	dirty = true;
	vertices[0].colour = v;
}

void Sprite::setScale(Vector2f v)
{
	dirty = true;
	scale = v;
	vertices[0].size = scale * size;
}

void Sprite::setOffset(Vector2f v)
{
	dirty = true;
	vertices[0].offset = v;
}

void Sprite::setSize(Vector2f v)
{
	dirty = true;
	size = v;
	vertices[0].size = scale * size;
}

void Sprite::setTexRect(Rect4f v)
{
	dirty = true;
	vertices[0].texRect = v;
}
