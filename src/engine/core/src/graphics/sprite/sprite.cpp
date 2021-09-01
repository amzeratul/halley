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

#include "api/video_api.h"
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
			slices *= sliceScale;

			painter.drawSlicedSprite(material, vertexAttrib.scale, slices, &vertexAttrib);
		});
	}
}

void Sprite::draw(gsl::span<const Sprite> sprites, Painter& painter) // static
{
	if (sprites.empty()) {
		return;
	}

	auto& material = sprites[0].material;
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib));

	size_t spriteSize = sizeof(SpriteVertexAttrib);
	char buffer[4096];
	char* vertexData;
	std::vector<char> vertices;
	const size_t vertexDataSize = sprites.size() * spriteSize;
	if (vertexDataSize <= 4096) {
		vertexData = buffer;
	} else {
		vertices.resize(vertexDataSize);
		vertexData = vertices.data();
	}

	for (size_t i = 0; i < sprites.size(); i++) {
		auto& sprite = sprites[i];
		Expects(sprite.material == material);
		memcpy(&vertexData[i * spriteSize], &sprite.vertexAttrib, spriteSize);
	}

	painter.drawSprites(material, sprites.size(), vertexData);
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
			draw(gsl::span<const Sprite>(sprites + start, i - start), painter);
			start = i;
			lastMaterial = material;
		}
	}
	draw(gsl::span<const Sprite>(sprites + start, n - start), painter);
}

Rect4f Sprite::getLocalAABB() const
{
	const Vector2f sz = getScaledSize() * Vector2f(flip ? -1.0f : 1.0f, 1.0f);
	const Vector2f pivot = getPivot();
	const auto offset = -sz * pivot;
	return Rect4f(offset, sz + offset);
}

Rect4f Sprite::getAABB() const
{
	// PERFORMANCE CRITICAL CODE
	
	const Vector2f sz = getScaledSize() * Vector2f(flip ? -1.0f : 1.0f, 1.0f);

	//Expects(!std::isnan(sz.x));
	//Expects(!std::isnan(sz.y));
	
	if (std::abs(getRotation().toRadians()) < 0.0001f) {
		// No rotation, give exact bounding box
		const Vector2f pivot = getPivot();
		const auto pos = getPosition();
		const auto offsetPos = pos - (sz * pivot);
		return Rect4f(offsetPos, offsetPos + sz);
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

Sprite& Sprite::setSliceScale(float scale)
{
	sliceScale = scale;
	return *this;
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
	mat->set(0, image);
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

Sprite& Sprite::setImage(Resources& resources, VideoAPI& videoAPI, std::shared_ptr<Image> image, String materialName)
{
	if (image && image->getSize().x > 0 && image->getSize().y > 0) {
		auto tex = std::shared_ptr(videoAPI.createTexture(image->getSize()));
		TextureDescriptor desc(image->getSize(), TextureFormat::RGBA);
		desc.pixelData = std::move(image);
		tex->startLoading();
		tex->load(std::move(desc));

		const auto matDef = resources.get<MaterialDefinition>(materialName.isEmpty() ? "Halley/Sprite" : materialName);
		setImage(tex, matDef);
		setImageData(*tex);
	}

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

bool Sprite::isPointVisible(Vector2f point) const
{
	// Is the sprite visible?
	if (!visible || getColour().a < 0.0001f) {
		return false;
	}

	// Compute local space point
	const auto localPoint = point - getPosition();

	// Check AABB first
	const auto aabb = getLocalAABB();
	if (!aabb.contains(localPoint)) {
		return false;
	}

	// Check against texture
	if (material) {
		const auto tex = material->getTexture(0);
		if (tex) {
			auto relPos = (localPoint - aabb.getTopLeft()) / aabb.getSize();
			if (flip ^ (getScale().x < 0)) {
				relPos.x = 1.0f - relPos.x;
			}
			if (getScale().y < 0) {
				relPos.y = 1.0f - relPos.y;
			}
			
			const auto texRect = getTexRect0();
			const auto texelPos = relPos * texRect.getSize() + texRect.getTopLeft();
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

Sprite& Sprite::crop(Vector4f sides)
{
	const auto scale = getScale();
	if (flip ^ (scale.x < 0)) {
		std::swap(sides.x, sides.z);
	}
	const auto absScale = scale.abs();
	sides /= Vector4f(absScale.x, absScale.y, absScale.x, absScale.y);
	
	const auto origSize = getSize();
	const auto origPivot = getAbsolutePivot();
	setSize(origSize - (sides.xy() + sides.zw()));
	setAbsolutePivot(origPivot - sides.xy());

	const auto texRect0 = vertexAttrib.texRect0;
	const auto texScale0 = texRect0.getSize() / origSize;
	vertexAttrib.texRect0 = Rect4f(texRect0.getTopLeft() + sides.xy() * texScale0, texRect0.getBottomRight() - sides.zw() * texScale0);
	const auto texRect1 = vertexAttrib.texRect1;
	const auto texScale1 = texRect1.getSize() / origSize;
	vertexAttrib.texRect1 = Rect4f(texRect1.getTopLeft() + sides.xy() * texScale1, texRect1.getBottomRight() - sides.zw() * texScale1);

	return *this;
}

Sprite::RectInfo Sprite::getRectInfo() const
{
	return RectInfo{ vertexAttrib.pivot, size, vertexAttrib.texRect0, vertexAttrib.texRect1 };
}

void Sprite::setRectInfo(const RectInfo& info)
{
	vertexAttrib.pivot = info.pivot;
	size = info.size;
	vertexAttrib.texRect0 = info.texRect0;
	vertexAttrib.texRect1 = info.texRect1;
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
	deserialize(context, node, sprite);
	return sprite;
}

void ConfigNodeSerializer<Sprite>::deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node, Sprite& sprite)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		return;
	}

	// Get the material definition
	std::shared_ptr<const MaterialDefinition> material;
	const auto& materialNode = node["material"];
	if (materialNode.getType() == ConfigNodeType::String) {
		material = context.resources->get<MaterialDefinition>(materialNode.asString());
	} else if (materialNode.getType() == ConfigNodeType::Del || !sprite.hasMaterial()) {
		material = context.resources->get<MaterialDefinition>("Halley/Sprite");
	} else {
		material = sprite.getMaterial().getDefinitionPtr();
	}

	auto loadTexture = [&](const String& nodeName, size_t texUnit) -> bool
	{
		if (!node.hasKey(nodeName)) {
			return false;
		}
		
		const auto& imageNode = node[nodeName];
		if (imageNode.getType() == ConfigNodeType::Del) {
			sprite.getMutableMaterial().set(texUnit, std::shared_ptr<const Texture>());
			return false;
		}
		
		if (texUnit == 0) {
			sprite.setImage(*context.resources, imageNode.asString(), node["material"].asString(sprite.hasMaterial() ? sprite.getMaterial().getDefinition().getName() : ""));
		} else {
			const auto image = context.resources->get<SpriteResource>(imageNode.asString());
			sprite.setTexRect1(image->getSprite().coords);
			sprite.getMutableMaterial().set(texUnit, image->getSpriteSheet()->getTexture());
		}
		return true;
	};

	if (material) {
		// Load each texture
		size_t i = 0;
		for (const auto& tex: material->getTextures()) {
			const bool loaded = loadTexture("tex_" + tex.name, i);
			if (!loaded) {
				if (i == 0) {
					loadTexture("image", i);
				} else if (i == 1) {
					loadTexture("image1", i);
				}
			}
			i++;
		}

		// Load material parameters
		for (const auto& block: material->getUniformBlocks()) {
			for (const auto& uniform: block.uniforms) {
				if (uniform.editable) {
					const auto key = "par_" + uniform.name;
					if (node.hasKey(key)) {
						const auto& parNode = node[key];
						if (uniform.type == ShaderParameterType::Float) {
							sprite.getMutableMaterial().set(uniform.name, parNode.asFloat());
						}
					}
				}
			}
		}
	}
	
	if (node.hasKey("pivot")) {
		const auto& pivotNode = node["pivot"];
		if (pivotNode.getType() == ConfigNodeType::Del) {
			sprite.setAbsolutePivot(Vector2f());
		} else {
			sprite.setAbsolutePivot(pivotNode.asVector2f());
		}
	}
	if (node.hasKey("flip")) {
		const auto& flipNode = node["flip"];
		if (flipNode.getType() == ConfigNodeType::Del) {
			sprite.setFlip(false);
		} else {
			sprite.setFlip(flipNode.asBool());
		}
	}
	if (node.hasKey("visible")) {
		const auto& visibleNode = node["visible"];
		if (visibleNode.getType() == ConfigNodeType::Del) {
			sprite.setVisible(true);
		} else {
			sprite.setVisible(node["visible"].asBool());
		}
	}
	if (node.hasKey("colour")) {
		const auto& colourNode = node["colour"];
		if (colourNode.getType() == ConfigNodeType::Del) {
			sprite.setColour(Colour(1, 1, 1));
		} else {
			sprite.setColour(Colour4f::fromString(node["colour"].asString()));
		}
	}
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
		material->set(0, sprite.getSpriteSheet()->getTexture());
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
	other.setHotReload(nullptr, 0);

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
	if (hotReloadRef != ref) {
		if (hotReloadRef) {
			hotReloadRef->removeSprite(this);
		}
		if (ref) {
			ref->addSprite(this, index);
		}
	} else if (hotReloadRef != nullptr && hotReloadIdx != index) {
		hotReloadRef->updateSpriteIndex(this, index);
	}
	hotReloadRef = ref;
	hotReloadIdx = index;
}
#endif
