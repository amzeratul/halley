#include <cstring>
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/graphics/texture.h"
#include "halley/resources/resources.h"
#include <gsl/assert>

#include "halley/api/video_api.h"
#include "halley/entity/entity_factory.h"
#include "halley/file_formats/config_file.h"
#include "halley/support/logger.h"

using namespace Halley;

Sprite::Sprite()
{
	visible = true;
	flip = false;
	hasClip = false;
	absoluteClip = false;
	sliced = false;
	rotated = false;
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
		Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib) + 16);

		paintWithClip(painter, extClip, [&] ()
		{
			painter.drawSprites(material, 1, getVertexAttrib());
		});
	}
}

void Sprite::drawSliced(Painter& painter, Vector4s slicesPixel, const std::optional<Rect4f>& extClip) const
{
	if (material) {
		Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib) + 16);
		
		paintWithClip(painter, extClip, [&] ()
		{
			Vector4f slices(slicesPixel);
			slices.x /= size.x;
			slices.y /= size.y;
			slices.z /= size.x;
			slices.w /= size.y;
			slices *= sliceScale;

			painter.drawSlicedSprite(material, vertexAttrib.scale, slices, getVertexAttrib());
		});
	}
}

void Sprite::draw(gsl::span<const Sprite> sprites, Painter& painter) // static
{
	if (sprites.empty()) {
		return;
	}

	auto& material = sprites[0].material;
	Expects(material->getDefinition().getVertexStride() == sizeof(SpriteVertexAttrib) + 16);

	size_t spriteSize = sizeof(SpriteVertexAttrib) + 16;
	char buffer[4096];
	char* vertexData;
	Vector<char> vertices;
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
		memcpy(&vertexData[i * spriteSize], sprite.getVertexAttrib(), spriteSize);
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
	
	if (!rotated) {
		// No rotation, give exact bounding box
		const Vector2f pivot = getPivot();
		const auto pos = getPosition();
		const auto offsetPos = pos - (sz * pivot);
		return Rect4f(offsetPos, offsetPos + sz);
	} else {
		// This is a coarse test; will give a few false positives
		constexpr float sqrt2 = 1.4142135623730950488016887242097f;
		const Vector2f sz2 = sz * sqrt2;
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
		constexpr float sqrt2 = 1.4142135623730950488016887242097f;
		const Vector2f sz2 = sz * sqrt2;
		return getPosition() + Rect4f(-sz2, sz2); // Could use offset here, but that would also need to take rotation into account
	}
}

bool Sprite::isInView(Rect4f rect) const
{
	if (!visible) {
		return false;
	}

	return getAABB().overlaps(rect);
}

Sprite& Sprite::setRotation(Angle1f v)
{
	vertexAttrib.rotation = v.getRadians();
	rotated = std::abs(v.getRadians()) > 0.000001f;
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
	return setScale(newSize / getUncroppedSize());
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
	
	vertexAttrib.texRect0 = texRect.toVector4();
	return *this;
}

Sprite& Sprite::setTexRect(Vector4f texRect)
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

Sprite& Sprite::setTexRect0(Vector4f texRect)
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
		materialName = MaterialDefinition::defaultMaterial;
	}
	setMaterial(resources.get<MaterialDefinition>(materialName)->getMaterial());
	return *this;
}

Sprite& Sprite::setMaterial(std::shared_ptr<const Material> m)
{
	Expects(m != nullptr);

	const bool hadMaterial = static_cast<bool>(material);
	material = std::move(m);

	if (!hadMaterial && material->getNumTextureUnits() > 0) {
		if (const auto& tex0 = material->getRawTexture(0)) {
			setImageData(*tex0);
		}
	}

	return *this;
}

MaterialUpdater Sprite::getMutableMaterial()
{
	return MaterialUpdater(material);
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

Sprite& Sprite::setImage(std::shared_ptr<const Texture> image, std::shared_ptr<const MaterialDefinition> materialDefinition)
{
	Expects(image != nullptr);
	Expects(materialDefinition != nullptr);

	auto mat = std::make_shared<Material>(materialDefinition);
	mat->set(0, image);
	setMaterial(mat);
	return *this;
}

Sprite& Sprite::setImage(Resources& resources, std::string_view imageName, std::string_view materialName)
{
	Expects (!imageName.empty());

	const auto sprite = resources.get<SpriteResource>(imageName);

	if (materialName.empty()) {
		materialName = sprite->getDefaultMaterialName();
	}

	auto mat = sprite->getMaterial(materialName);
	mat->set(0, *sprite);
	setMaterial(mat);
	doSetSprite(sprite->getSprite(), true);
	
#ifdef ENABLE_HOT_RELOAD
	setHotReload(sprite.get(), 0);
#endif

	return *this;
}

Sprite& Sprite::setImage(const SpriteResource& sprite, std::shared_ptr<const MaterialDefinition> materialDefinition)
{
	auto mat = std::make_shared<Material>(materialDefinition);
	mat->set(0, sprite);
	setMaterial(mat);
	doSetSprite(sprite.getSprite(), true);
	
#ifdef ENABLE_HOT_RELOAD
	setHotReload(&sprite, 0);
#endif
	
	return *this;
}

Sprite& Sprite::setImage(Resources& resources, VideoAPI& videoAPI, std::shared_ptr<Image> image, std::string_view materialName)
{
	if (image && image->getSize().x > 0 && image->getSize().y > 0) {
		auto tex = std::shared_ptr<Texture>(videoAPI.createTexture(image->getSize()));
		tex->setAssetId(image->getAssetId());
		TextureDescriptor desc(image->getSize(), TextureFormat::RGBA);
		desc.pixelData = std::move(image);
		tex->startLoading();
		tex->load(std::move(desc));

		const auto matDef = resources.get<MaterialDefinition>(materialName.empty() ? MaterialDefinition::defaultMaterial : materialName);
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

Sprite& Sprite::setSprite(Resources& resources, std::string_view spriteSheetName, std::string_view imageName, std::string_view materialName)
{
	Expects (!spriteSheetName.empty());
	Expects (!imageName.empty());

	if (materialName.empty()) {
		materialName = MaterialDefinition::defaultMaterial;
	}
	auto spriteSheet = resources.get<SpriteSheet>(spriteSheetName);
	setMaterial(spriteSheet->getMaterial(materialName));
	setSprite(spriteSheet->getSprite(imageName), true);
		
	return *this;
}

Sprite& Sprite::setSprite(const SpriteSheet& sheet, std::string_view name, bool applyPivot)
{
	Expects (!name.empty());
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
	vertexAttrib.texRect0 = entry.coords.toVector4();
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

bool Sprite::hasPointVisible(Rect4f area) const
{
	// Is the sprite visible?
	if (!visible || getColour().a < 0.0001f) {
		return false;
	}

	// Compute local space point
	const auto localArea = area - getPosition();

	// Check AABB first
	const auto aabb = getLocalAABB();
	if (aabb.isEmpty() || !aabb.overlaps(localArea)) {
		return false;
	}

	// If the local area completely envelopes the aabb, then there's no need to check for pixels, we'll consider it to count
	if (localArea.contains(aabb)) {
		return true;
	}

	// Check against texture
	if (material) {
		const auto tex = material->getTexture(0);
		if (tex) {
			auto overlapArea = localArea.intersection(aabb);

			Rect4f relPos = (overlapArea - aabb.getTopLeft()) / aabb.getSize();
			if (flip ^ (getScale().x < 0)) {
				relPos.getP1().x = 1.0f - relPos.getP1().x;
				relPos.getP2().x = 1.0f - relPos.getP2().x;
			}
			if (getScale().y < 0) {
				relPos.getP1().y = 1.0f - relPos.getP1().y;
				relPos.getP2().y = 1.0f - relPos.getP2().y;
			}
			
			const auto texRect = getTexRect0();
			const auto texelRect = relPos.mult(texRect.getSize()) + texRect.getTopLeft(); // lol hack
			const auto pixelRect = texelRect.mult(Vector2f(tex->getSize()));
			const auto pixelBounds = Rect4i(Vector2i(pixelRect.getTopLeft().floor()), Vector2i(pixelRect.getBottomRight().floor()));
			return tex->hasOpaquePixels(pixelBounds);
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
	const auto texScale0 = Rect4f(texRect0).getSize() / origSize;
	vertexAttrib.texRect0 = texRect0 + Vector4f(texScale0 * sides.xy(), -texScale0 * sides.zw());
	const auto texRect1 = vertexAttrib.texRect1;
	const auto texScale1 = Rect4f(texRect1).getSize() / origSize;
	vertexAttrib.texRect1 = texRect1 + Vector4f(texScale1 * sides.xy(), -texScale1 * sides.zw());

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
	vertexAttrib.texRect0 = info.texRect0.toVector4();
	vertexAttrib.texRect1 = info.texRect1.toVector4();
	computeSize();
}

Sprite Sprite::clone(bool enableHotReload) const
{
#ifdef ENABLE_HOT_RELOAD
	if (enableHotReload) {
		return *this;
	} else {
		Sprite result;
		result.copyFrom(*this, false);
		return result;
	}
#else
	return *this;
#endif
}

Sprite&& Sprite::move(bool enableHotReload)
{
#ifdef ENABLE_HOT_RELOAD
	if (!enableHotReload) {
		setHotReload(nullptr, 0);
	}
#endif
	return std::move(*this);
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

const void* Sprite::getVertexAttrib() const
{
	return reinterpret_cast<const char*>(&vertexAttrib) - sizeof(Vector4f);
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

ConfigNode ConfigNodeSerializer<Sprite>::serialize(const Sprite& sprite, const EntitySerializationContext& context)
{
	ConfigNode node = ConfigNode::MapType();
	if (sprite.getColour() != Colour4f(1, 1, 1, 1)) {
		node["colour"] = sprite.getColour().toString();
	}
	if (!sprite.isVisible()) {
		node["visible"] = false;
	}
	if (sprite.hasMaterial()) {
		const auto& materialDef = sprite.getMaterial().getDefinition();
		if (materialDef.getAssetId() != MaterialDefinition::defaultMaterial) {
			node["material"] = materialDef.getAssetId();
		}
		const auto& texs = materialDef.getTextures();
		for (size_t i = 0; i < texs.size(); ++i) {
			const auto& assetId = sprite.getMaterial().getTexUnitAssetId(static_cast<int>(i));
			const auto& texDef = texs[i];
			if (!assetId.isEmpty() && assetId != texDef.name && assetId != texDef.defaultTextureName) {
				node["tex_" + texDef.name] = assetId;
			}
		}
		for (const auto& ub: materialDef.getUniformBlocks()) {
			for (const auto& u: ub.uniforms) {
				// ?
			}
		}
	}
	return node;
}

Sprite ConfigNodeSerializer<Sprite>::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	Sprite sprite;
	deserialize(context, node, sprite);
	return sprite;
}

namespace {
	float clampToRange(std::optional<Range<float>> range, float v)
	{
		return range ? clamp(v, range->start, range->end) : v;
	}

	Vector2f clampToRange(std::optional<Range<float>> range, Vector2f v)
	{
		if (range) {
			return Vector2f(clamp(v.x, range->start, range->end), clamp(v.y, range->start, range->end));
		}
		return v;
	}

	Vector3f clampToRange(std::optional<Range<float>> range, Vector3f v)
	{
		if (range) {
			return Vector3f(clamp(v.x, range->start, range->end), clamp(v.y, range->start, range->end), clamp(v.z, range->start, range->end));
		}
		return v;
	}

	Vector4f clampToRange(std::optional<Range<float>> range, Vector4f v)
	{
		if (range) {
			return Vector4f(clamp(v.x, range->start, range->end), clamp(v.y, range->start, range->end), clamp(v.z, range->start, range->end), clamp(v.w, range->start, range->end));
		}
		return v;
	}
}

void ConfigNodeSerializer<Sprite>::deserialize(const EntitySerializationContext& context, const ConfigNode& node, Sprite& sprite)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		return;
	}
	if (context.entityContext && context.entityContext->isHeadless()) {
		return;
	}

	// Get the material definition
	bool hasNewMaterial = false;
	std::shared_ptr<const MaterialDefinition> materialDefinition;
	const auto& materialNode = node["material"];
	if (materialNode.getType() == ConfigNodeType::String) {
		materialDefinition = context.resources->get<MaterialDefinition>(materialNode.asStringView());
		hasNewMaterial = true;
	} else if (materialNode.getType() == ConfigNodeType::Del || !sprite.hasMaterial()) {
		materialDefinition = context.resources->get<MaterialDefinition>(MaterialDefinition::defaultMaterial);
		hasNewMaterial = true;
	} else {
		materialDefinition = sprite.getMaterial().getDefinitionPtr();
	}

	auto loadTexture = [&](std::string_view nodeName, size_t texUnit) -> bool
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
			sprite.setImage(*context.resources, imageNode.asStringView(), node["material"].asStringView(sprite.hasMaterial() ? std::string_view(sprite.getMaterial().getDefinition().getName()) : ""));
		} else {
			const auto image = context.resources->get<SpriteResource>(imageNode.asStringView());
			sprite.setTexRect1(image->getSprite().coords);
			sprite.getMutableMaterial().set(texUnit, *image);
		}
		return true;
	};

	if (materialDefinition) {
		if (materialDefinition->getTextures().empty()) {
			if (hasNewMaterial) {
				sprite.setMaterial(materialDefinition->getMaterial());
			}
		} else {
			// Load each texture
			size_t i = 0;
			for (const auto& tex: materialDefinition->getTextures()) {
				const bool loaded = loadTexture("tex_" + tex.name, i);
				if (!loaded) {
					if (i == 0) {
						if (!loadTexture("image", i)) {
							if (hasNewMaterial) {
								sprite.setMaterial(materialDefinition->getMaterial());
							}
						}
					} else if (i == 1) {
						loadTexture("image1", i);
					}
				}
				i++;
			}
		}

		// Load material parameters
		for (const auto& block: materialDefinition->getUniformBlocks()) {
			for (const auto& uniform: block.uniforms) {
				auto applyValue = [&] (const ConfigNode& node)
				{
					if (uniform.type == ShaderParameterType::Float) {
						sprite.getMutableMaterial().set(uniform.name, clampToRange(uniform.range, node.asFloat(0)));
					} else if (uniform.type == ShaderParameterType::Float2) {
						sprite.getMutableMaterial().set(uniform.name, clampToRange(uniform.range, node.asVector2f(Vector2f())));
					} else if (uniform.type == ShaderParameterType::Float3) {
						sprite.getMutableMaterial().set(uniform.name, clampToRange(uniform.range, node.asVector3f(Vector3f())));
					} else if (uniform.type == ShaderParameterType::Float4) {
						sprite.getMutableMaterial().set(uniform.name, clampToRange(uniform.range, node.asVector4f(Vector4f())));
					}
				};

				if (uniform.editable) {
					const auto key = "par_" + uniform.name;
					if (node.hasKey(key)) {
						applyValue(node[key]);
					} else {
						applyValue(uniform.defaultValue);
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
			sprite.setColour(Colour4f::fromString(node["colour"].asStringView()));
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
	setMaterial(copyMaterialParameters(material, sprite.getMaterial(material->getDefinition().getName())));
	doSetSprite(sprite.getSprite(), lastAppliedPivot);
}

std::shared_ptr<Material> Sprite::copyMaterialParameters(const std::shared_ptr<const Material>& oldMaterial, std::shared_ptr<Material> newMaterial)
{
	Vector<size_t> compatibleBlocks;

	const size_t nBlocks = newMaterial->getDataBlocks().size();
	if (oldMaterial->getDataBlocks().size() == nBlocks) {
		for (size_t i = 0; i < nBlocks; ++i) {
			const auto& oldBlock = oldMaterial->getDataBlocks()[i];
			const auto& newBlock = newMaterial->getDataBlocks()[i];
			if (oldBlock.getType() == MaterialDataBlockType::Local && newBlock.getType() == MaterialDataBlockType::Local) {
				if (oldBlock.getData().size() == newBlock.getData().size() && memcmp(oldBlock.getData().data(), newBlock.getData().data(), newBlock.getData().size()) != 0) {
					compatibleBlocks.push_back(i);
				}
			}
		}
	}

	if (compatibleBlocks.empty()) {
		return newMaterial;
	}

	auto result = newMaterial->clone();

	for (auto i: compatibleBlocks) {
		const auto& oldBlock = oldMaterial->getDataBlocks()[i];
		auto& newBlock = result->getDataBlocks()[i];
		newBlock = oldBlock;
	}

	return result;
}

Sprite::Sprite(const Sprite& other)
{
	*this = other;
}

Sprite::Sprite(Sprite&& other) noexcept
{
	*this = std::move(other);
}

void Sprite::copyFrom(const Sprite& other, bool enableHotReload)
{
	if (this == &other) {
		return;
	}

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
	rotated = other.rotated;
	lastAppliedPivot = other.lastAppliedPivot;

#ifdef ENABLE_HOT_RELOAD
	if (enableHotReload) {
		setHotReload(other.hotReloadRef, other.hotReloadIdx);
	} else {
		setHotReload(nullptr, 0);
	}
#endif
}

void Sprite::moveFrom(Sprite&& other, bool enableHotReload)
{
	if (this == &other) {
		return;
	}

	vertexAttrib = other.vertexAttrib;
	material = std::move(other.material);
	size = other.size;
	slices = other.slices;
	outerBorder = other.outerBorder;
	clip = other.clip;
	hasClip = other.hasClip;
	absoluteClip = other.absoluteClip;
	visible = other.visible;
	flip = other.flip;
	sliced = other.sliced;
	rotated = other.rotated;
	lastAppliedPivot = other.lastAppliedPivot;

#ifdef ENABLE_HOT_RELOAD
	if (enableHotReload) {
		setHotReload(other.hotReloadRef, other.hotReloadIdx);
	} else {
		setHotReload(nullptr, 0);
	}
	other.setHotReload(nullptr, 0);
#endif
}

Sprite& Sprite::operator=(const Sprite& other)
{
	copyFrom(other);
	return *this;
}

Sprite& Sprite::operator=(Sprite&& other) noexcept
{
	moveFrom(std::move(other));
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
