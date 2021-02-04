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
#include "halley/support/logger.h"

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
	if (material) {
		Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

		paintWithClip(painter, extClip, [&] ()
		{
			painter.drawSprites(material, 1, &vertexAttrib);
		});
	}
}

void Sprite::drawSliced(Painter& painter, Vector4s slicesPixel, const std::optional<Rect4f>& extClip) const
{
	if (material) {
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
	const Vector2f sz = getScaledSize() * Vector2f(flip ? -1.0f : 1.0f, 1.0f);
	const Vector2f pivot = getPivot();
	return Rect4f(-sz * pivot, sz * (Vector2f(1, 1) - pivot));
}

Rect4f Sprite::getAABB() const
{
	const Vector2f sz = getScaledSize() * Vector2f(flip ? -1.0f : 1.0f, 1.0f);

	Expects(!std::isnan(sz.x));
	Expects(!std::isnan(sz.y));
	
	if (std::abs(getRotation().toRadians()) < 0.0001f) {
		// No rotation, give exact bounding box
		const Vector2f pivot = getPivot();
		auto aabb = getPosition() + Rect4f(-sz * pivot, sz * (Vector2f(1, 1) - pivot));
		//Ensures(!std::isnan(aabb.getWidth()));
		//Ensures(!std::isnan(aabb.getHeight()));
		return aabb;
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

Sprite& Sprite::setRotation(Angle1f v)
{
	vertexAttrib.rotation = v.getRadians();
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

Sprite& Sprite::setTexRect(Rect4f texRect)
{
#ifdef ENABLE_HOT_RELOAD
	setHotReload(nullptr, 0);
#endif
	
	vertexAttrib.texRect0 = texRect;
	return *this;
}

Sprite& Sprite::setTexRect0(Rect4f texRect)
{
	return setTexRect(texRect);
}

Sprite& Sprite::setPivot(Vector2f v)
{
	Expects(v.isValid());
	
	vertexAttrib.pivot = v;

	return *this;
}

Sprite& Sprite::setAbsolutePivot(Vector2f v)
{
	vertexAttrib.pivot = v / size;

	if (std::abs(size.x) < 0.000001f) {
		vertexAttrib.pivot.x = 0;
	}
	if (std::abs(size.y) < 0.000001f) {
		vertexAttrib.pivot.y = 0;
	}
	
	Ensures(vertexAttrib.pivot.isValid());

	return *this;
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
	return *getMutableMaterialPtr();
}

std::shared_ptr<Material> Sprite::getMutableMaterialPtr()
{
	Expects(material);
	if (sharedMaterial) {
		material = material->clone();
		sharedMaterial = false;
	}
	return material;
}

bool Sprite::hasCompatibleMaterial(const Material& other) const
{
	if (!material) {
		return false;
	}
	return material->isCompatibleWith(other);
}

Sprite& Sprite::setImageData(const Texture& image)
{
	setSize(Vector2f(image.getSize()));
	setTexRect0(Rect4f(0, 0, 1, 1));
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
	doSetSprite(sprite->getSprite(), true);
		
#ifdef ENABLE_HOT_RELOAD
	setHotReload(sprite.get(), 0);
#endif
	
	return *this;
}

Sprite& Sprite::setImage(const SpriteResource& sprite, std::shared_ptr<const MaterialDefinition> materialDefinition, bool shared)
{
	const auto spriteSheet = sprite.getSpriteSheet();
	setImage(spriteSheet->getTexture(), std::move(materialDefinition), shared);
	doSetSprite(sprite.getSprite(), true);
	
#ifdef ENABLE_HOT_RELOAD
	setHotReload(&sprite, 0);
#endif
	
	return *this;
}

Sprite& Sprite::setSprite(const SpriteResource& sprite, bool applyPivot)
{
#ifdef ENABLE_HOT_RELOAD
	setHotReload(&sprite, 0);
#endif

	doSetSprite(sprite.getSprite(), applyPivot);
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
	setSprite(spriteSheet->getSprite(imageName), true);
		
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
	doSetSprite(entry, applyPivot);

#ifdef ENABLE_HOT_RELOAD
	setHotReload(entry.parent, entry.idx);
#endif

	return *this;
}

void Sprite::doSetSprite(const SpriteSheetEntry& entry, bool applyPivot)
{
	outerBorder = entry.trimBorder;
	slices = entry.slices;
	sliced = slices.x != 0 || slices.y != 0 || slices.z != 0 || slices.w != 0;
	setSize(entry.size);
	if (applyPivot) {
		Expects(entry.pivot.isValid());
		vertexAttrib.pivot = entry.pivot;
	}
	vertexAttrib.texRect0 = entry.coords;
	vertexAttrib.textureRotation = entry.rotated ? 1.0f : 0.0f;

#ifdef ENABLE_HOT_RELOAD
	lastAppliedPivot = applyPivot;
#endif
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
			const auto texRect = getTexRect0();
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

bool Sprite::operator==(const Sprite& other) const
{
	// TODO?
	return false;
}

bool Sprite::operator!=(const Sprite& other) const
{
	return !(*this == other);
}

void Sprite::computeSize()
{
	vertexAttrib.size = size;
	if (flip) {
		vertexAttrib.size.x *= -1;
	}
}

Vector2f Sprite::getUncroppedSize() const
{
	return getSize() + Vector2f(outerBorder.xy() + outerBorder.zw());
}

Vector2f Sprite::getUncroppedScaledSize() const
{
	return getScale() * getUncroppedSize();
}

Sprite& Sprite::setOuterBorder(Vector4s border)
{
	outerBorder = border;
	return *this;
}

ConfigNode ConfigNodeSerializer<Sprite>::serialize(const Sprite& sprite, const ConfigNodeSerializationContext& context)
{
	// TODO
	// How do I even do this, though...
	ConfigNode node;
	return node;
}

Sprite ConfigNodeSerializer<Sprite>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	Sprite sprite;
	if (node.getType() == ConfigNodeType::Undefined) {
		return sprite;
	}

	if (node.hasKey("image")) {
		sprite.setImage(*context.resources, node["image"].asString(), node["material"].asString(""));
	}
	if (node.hasKey("image1")) {
		const auto image1 = context.resources->get<SpriteResource>(node["image1"].asString());
		sprite.setTexRect1(image1->getSprite().coords);
		sprite.getMutableMaterial().set("tex1", image1->getSpriteSheet()->getTexture());
	}
	if (node.hasKey("pivot")) {
		sprite.setAbsolutePivot(node["pivot"].asVector2f());
	}
	sprite.setFlip(node["flip"].asBool(false));
	sprite.setVisible(node["visible"].asBool(true));
	sprite.setColour(Colour4f::fromString(node["colour"].asString("#FFFFFF")));

	return sprite;
}


#ifdef ENABLE_HOT_RELOAD
Sprite::~Sprite()
{
	setHotReload(nullptr, 0);
}

void Sprite::reloadSprite(const SpriteResource& sprite)
{
	if (sharedMaterial) {
		setMaterial(sprite.getMaterial(material->getDefinition().getName()));
	} else if (material) {
		material->set("tex0", sprite.getSpriteSheet()->getTexture());
	}
	doSetSprite(sprite.getSprite(), lastAppliedPivot);
}

Sprite::Sprite(const Sprite& other)
{
	*this = other;
}

Sprite::Sprite(Sprite&& other) noexcept
{
	*this = other;
}

Sprite& Sprite::operator=(const Sprite& other)
{
	vertexAttrib = other.vertexAttrib;
	material = other.material;
	size = other.size;
	slices = other.slices;
	outerBorder = other.outerBorder;
	clip = other.clip;
	hasClip = other.hasClip;
	absoluteClip = other.absoluteClip;
	visible = other.visible;
	flip = other.flip;
	sliced = other.sliced;
	sharedMaterial = other.sharedMaterial;
	lastAppliedPivot = other.lastAppliedPivot;
	
	setHotReload(other.hotReloadRef, other.hotReloadIdx);
	
	return *this;
}

Sprite& Sprite::operator=(Sprite&& other) noexcept
{
	vertexAttrib = std::move(other.vertexAttrib);
	material = std::move(other.material);
	size = std::move(other.size);
	slices = std::move(other.slices);
	outerBorder = std::move(other.outerBorder);
	clip = std::move(other.clip);
	hasClip = std::move(other.hasClip);
	absoluteClip = std::move(other.absoluteClip);
	visible = std::move(other.visible);
	flip = std::move(other.flip);
	sliced = std::move(other.sliced);
	sharedMaterial = std::move(other.sharedMaterial);
	lastAppliedPivot = std::move(other.lastAppliedPivot);

	setHotReload(other.hotReloadRef, other.hotReloadIdx);

	return *this;
}

bool Sprite::hasLastAppliedPivot() const
{
	return lastAppliedPivot;
}

void Sprite::clearSpriteSheetRef()
{
	hotReloadRef = nullptr;
	hotReloadIdx = 0;
}

void Sprite::setHotReload(const SpriteHotReloader* ref, uint32_t index)
{
	if (hotReloadRef != ref || hotReloadIdx != index) {
		if (hotReloadRef) {
			hotReloadRef->removeSprite(this);
		}
		if (ref) {
			ref->addSprite(this, index);
		}
		hotReloadRef = ref;
		hotReloadIdx = index;
	}
}
#endif
