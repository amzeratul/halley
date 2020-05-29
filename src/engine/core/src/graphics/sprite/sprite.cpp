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

#include "halley/file_formats/config_file.h"

using namespace Halley;

Sprite::Sprite()
{
	setScale(Vector2f(1, 1));
	setColour(Colour4f(1, 1, 1, 1));
}

template <typename F>
void Sprite::paintWithClip(Painter& painter, const std::optional<Rect4f>& extClip, F f) const
{
	const bool needsClip = hasClip || extClip;
	if (needsClip) {
		const Rect4f finalClip = Rect4f::optionalIntersect(getAbsoluteClip(), extClip).value();

		const auto onScreen = getAABB().intersection(finalClip);
		if (onScreen.getWidth() < 0.01f || onScreen.getHeight() < 0.01f) {
			// Invisible, abort drawing
			return;
		}
		
		painter.setRelativeClip(finalClip);
	}

	f();

	if (needsClip) {
		painter.setClip();
	}
}

void Sprite::draw(Painter& painter, const std::optional<Rect4f>& extClip) const
{
	if (sliced) {
		drawSliced(painter, slices, extClip);
	} else {
		drawNormal(painter, extClip);
	}
}

void Sprite::drawSliced(Painter& painter, const std::optional<Rect4f>& extClip) const
{
	drawSliced(painter, slices, extClip);
}

void Sprite::drawNormal(Painter& painter, const std::optional<Rect4f>& extClip) const
{
	Expects(material != nullptr);
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

	paintWithClip(painter, extClip, [&] ()
	{
		painter.drawSprites(material, 1, &vertexAttrib);
	});
}

void Sprite::drawSliced(Painter& painter, Vector4s slicesPixel, const std::optional<Rect4f>& extClip) const
{
	Expects(material != nullptr);
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));
	
	paintWithClip(painter, extClip, [&] ()
	{
		Vector4f slices(slicesPixel);
		slices.x /= size.x;
		slices.y /= size.y;
		slices.z /= size.x;
		slices.w /= size.y;

		painter.drawSlicedSprite(material, vertexAttrib.scale, slices, &vertexAttrib);
	});
}

void Sprite::draw(const Sprite* sprites, size_t n, Painter& painter) // static
{
	if (n == 0) {
		return;
	}

	auto& material = sprites[0].material;
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

	size_t spriteSize = sizeof(SpriteVertexAttrib);
	char buffer[4096];
	char* vertexData;
	std::vector<char> vertices;
	const size_t vertexDataSize = n * spriteSize;
	if (vertexDataSize <= 4096) {
		vertexData = buffer;
	} else {
		vertices.resize(vertexDataSize);
		vertexData = vertices.data();
	}

	for (size_t i = 0; i < n; i++) {
		auto& sprite = sprites[i];
		Expects(sprite.material == material);
		memcpy(&vertexData[i * spriteSize], &sprite.vertexAttrib, spriteSize);
	}

	painter.drawSprites(material, n, vertexData);
}

void Sprite::drawMixedMaterials(const Sprite* sprites, size_t n, Painter& painter)
{
	if (n == 0) {
		return;
	}

	size_t start = 0;
	auto* lastMaterial = sprites[0].material.get();
	for (size_t i = 0; i < n; ++i) {
		auto* material = sprites[i].material.get();
		if (material != lastMaterial) {
			draw(sprites + start, i - start, painter);
			start = i;
			lastMaterial = material;
		}
	}
	draw(sprites + start, n - start, painter);
}

Rect4f Sprite::getLocalAABB() const
{
	const Vector2f sz = getSize();
	const Vector2f pivot = getPivot();
	return Rect4f(-sz * pivot, sz * (Vector2f(1, 1) - pivot));
}

Rect4f Sprite::getAABB() const
{
	const Vector2f sz = getScaledSize() * Vector2f(flip ? -1.0f : 1.0f, 1.0f);
	if (std::abs(getRotation().toRadians()) < 0.0001f) {
		// No rotation, give exact bounding box
		const Vector2f pivot = getPivot();
		return getPosition() + Rect4f(-sz * pivot, sz * (Vector2f(1, 1) - pivot));
	} else {
		// This is a coarse test; will give a few false positives
		const Vector2f sz2 = sz * std::sqrt(2);
		return getPosition() + Rect4f(-sz2, sz2); // Could use offset here, but that would also need to take rotation into account
	}
}

Rect4f Sprite::getUncroppedAABB() const
{
	const Vector2f sz = getUncroppedScaledSize();
	if (std::abs(getRotation().toRadians()) < 0.0001f) {
		// No rotation, give exact bounding box
		const Vector2f pivot = getUncroppedAbsolutePivot();
		return getPosition() - pivot + Rect4f(Vector2f(), sz);
	} else {
		// This is a coarse test; will give a few false positives
		const Vector2f sz2 = sz * std::sqrt(2);
		return getPosition() + Rect4f(-sz2, sz2); // Could use offset here, but that would also need to take rotation into account
	}
}

bool Sprite::isInView(Rect4f v) const
{
	return isVisible() && getAABB().overlaps(v);
}

Vector2f Sprite::getScaledSize() const
{
	return getScale() * getSize();
}

Sprite& Sprite::setRotation(Angle1f v)
{
	vertexAttrib.rotation = v.getRadians();
	return *this;
}

Angle1f Sprite::getRotation() const
{
	return vertexAttrib.rotation;
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
	return getPivot() * size;
}

Vector2f Sprite::getUncroppedAbsolutePivot() const
{
	return getAbsolutePivot() + Vector2f(outerBorder.xy());
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

Rect4f Sprite::getTexRect() const
{
	return vertexAttrib.texRect;
}

Sprite& Sprite::setMaterial(Resources& resources, String materialName)
{
	if (materialName == "") {
		materialName = "Halley/Sprite";
	}
	setMaterial(std::make_shared<Material>(resources.get<MaterialDefinition>(materialName)), false);
	return *this;
}

Sprite& Sprite::setMaterial(std::shared_ptr<Material> m, bool shared)
{
	Expects(m != nullptr);

	const bool hadMaterial = static_cast<bool>(material);
	material = std::move(m);
	sharedMaterial = shared;

	if (!hadMaterial && !material->getTextures().empty()) {
		setImageData(*material->getTextures()[0]);
	}

	return *this;
}

Sprite& Sprite::setMaterial(std::unique_ptr<Material> m)
{
	setMaterial(std::move(m), false);
	return *this;
}

Material& Sprite::getMutableMaterial()
{
	if (sharedMaterial) {
		material = material->clone();
		sharedMaterial = false;
	}
	return *material;
}

Sprite& Sprite::setImageData(const Texture& image)
{
	setSize(Vector2f(image.getSize()));
	setTexRect(Rect4f(0, 0, 1, 1));
	return *this;
}

Sprite& Sprite::setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> materialDefinition, bool shared)
{
	Expects(image != nullptr);
	Expects(materialDefinition != nullptr);

	auto mat = std::make_shared<Material>(materialDefinition);
	mat->set("tex0", image);
	setMaterial(mat, shared);
	return *this;
}

Sprite& Sprite::setImage(Resources& resources, const String& imageName, String materialName)
{
	Expects (!imageName.isEmpty());
		
	const auto sprite = resources.get<SpriteResource>(imageName);

	if (materialName == "") {
		materialName = sprite->getDefaultMaterialName();
	}
	
	setMaterial(sprite->getMaterial(materialName), true);
	setSprite(sprite->getSprite());
	return *this;
}

Sprite& Sprite::setImage(const SpriteResource& sprite, std::shared_ptr<const MaterialDefinition> materialDefinition, bool shared)
{
	const auto spriteSheet = sprite.getSpriteSheet();
	setImage(spriteSheet->getTexture(), std::move(materialDefinition), shared);
	setSprite(sprite.getSprite());
	return *this;
}

Sprite& Sprite::setSprite(Resources& resources, const String& spriteSheetName, const String& imageName, String materialName)
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

Sprite& Sprite::setSprite(const SpriteSheet& sheet, const String& name, bool applyPivot)
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

bool Sprite::isSliced() const
{
	return sliced;
}

Vector4s Sprite::getSlices() const
{
	return slices;
}

bool Sprite::isPointVisible(Vector2f localPoint) const
{
	// Is the sprite visible?
	if (!visible || getColour().a == 0) {
		return false;
	}

	// Check AABB first
	if (!getLocalAABB().contains(localPoint)) {
		return false;
	}

	// Check against texture
	if (material) {
		const auto tex = material->getTexture(0);
		if (tex) {
			const auto rectPos = localPoint + getAbsolutePivot();
			const auto texRect = getTexRect();
			const auto texelPos = (rectPos / size) * texRect.getSize() + texRect.getTopLeft();
			const auto px = tex->getPixel(texelPos);
			if (px) {
				const auto pxColour = Image::convertIntToColour(px.value());
				return pxColour.a > 0.01f;
			}
		}
	}

	return true;
}

Sprite& Sprite::setClip(Rect4f c)
{
	clip = std::move(c);
	hasClip = true;
	absoluteClip = false;
	return *this;
}

Sprite& Sprite::setAbsoluteClip(Rect4f c)
{
	clip = std::move(c);
	hasClip = true;
	absoluteClip = true;
	return *this;
}

Sprite& Sprite::setClip()
{
	hasClip = false;
	absoluteClip = false;
	return *this;
}

std::optional<Rect4f> Sprite::getClip() const
{
	return hasClip ? clip : std::optional<Rect4f>();
}

std::optional<Rect4f> Sprite::getAbsoluteClip() const
{
	return hasClip ? clip + (absoluteClip ? Vector2f() : vertexAttrib.pos) : std::optional<Rect4f>();
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

Vector2f Sprite::getUncroppedSize() const
{
	return getSize() + Vector2f(outerBorder.xy() + outerBorder.zw());
}

Vector2f Sprite::getUncroppedScaledSize() const
{
	return getScale() * getUncroppedSize();
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

Sprite ConfigNodeSerializer<Sprite>::deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	Sprite sprite;

	if (node.hasKey("image")) {
		sprite.setImage(*context.resources, node["image"].asString(), node["material"].asString(""));
	}
	if (node.hasKey("pivot")) {
		sprite.setAbsolutePivot(node["pivot"].asVector2f());
	}
	sprite.setFlip(node["flip"].asBool(false));
	sprite.setVisible(node["visible"].asBool(true));
	sprite.setColour(Colour4f::fromString(node["colour"].asString("#FFFFFF")));

	return sprite;
}
