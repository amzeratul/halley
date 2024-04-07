#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/texture.h"
#include "halley/api/halley_api.h"
#include "halley/resources/resources.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/bin_pack.h"
#include "halley/support/logger.h"
#include "halley/utils/hash.h"

using namespace Halley;

SpriteSheetEntry::SpriteSheetEntry(const ConfigNode& node)
{
	pivot = node["pivot"].asVector2f({});
	origPivot = node["origPivot"].asVector2i({});
	size = node["size"].asVector2f();
	coords = node["coords"].asRect4f({});
	rotated = node["rotated"].asBool(false);
	trimBorder = Vector4s(node["trimBorder"].asVector4i({}));
	slices = Vector4s(node["slices"].asVector4i({}));
}

ConfigNode SpriteSheetEntry::toConfigNode() const
{
	ConfigNode::MapType result;
	result["pivot"] = pivot;
	result["origPivot"] = origPivot;
	result["size"] = size;
	result["coords"] = coords;
	result["rotated"] = rotated;
	result["trimBorder"] = Vector4i(trimBorder);
	result["slices"] = Vector4i(slices);
	return result;
}

void SpriteSheetEntry::serialize(Serializer& s) const
{
	Expects(pivot.isValid());
	Expects(size.isValid());
	
	s << pivot;
	s << origPivot;
	s << size;
	s << coords;
	s << rotated;
	s << trimBorder;
	s << slices;
}

void SpriteSheetEntry::deserialize(Deserializer& s)
{
	s >> pivot;
	s >> origPivot;
	s >> size;
	s >> coords;
	s >> rotated;
	s >> trimBorder;
	s >> slices;

	Ensures(pivot.isValid());
	Ensures(size.isValid());
}

void SpriteSheetFrameTag::serialize(Serializer& s) const
{
	s << name;
	s << to;
	s << from;
}

void SpriteSheetFrameTag::deserialize(Deserializer& s)
{
	s >> name;
	s >> to;
	s >> from;
}

SpriteSheet::SpriteSheet()
	: defaultMaterialName(MaterialDefinition::defaultMaterial)
{
}

SpriteSheet::~SpriteSheet()
{
#ifdef ENABLE_HOT_RELOAD
	clearSpriteRefs();
#endif
}

void SpriteSheet::load(const ConfigNode& node)
{
	textureName = node["textureName"].asString("");
	defaultMaterialName = node["defaultMaterialName"].asString();
	sprites = node["sprites"].asVector<SpriteSheetEntry>({});
	if (node.hasKey("spriteIdx")) {
		spriteIdx = node["spriteIdx"].asHashMap<String, uint32_t>();
	} else {
		spriteIdx.clear();
	}
	// TODO: frameTags
}

ConfigNode SpriteSheet::toConfigNode() const
{
	ConfigNode::MapType result;
	result["textureName"] = textureName;
	result["defaultMaterialName"] = defaultMaterialName;
	result["sprites"] = sprites;
	result["spriteIdx"] = spriteIdx;
	// TODO: frameTags
	return result;
}

const std::shared_ptr<const Texture>& SpriteSheet::getTexture() const
{
	Expects(resources != nullptr);
	if (!texture) {
		loadTexture(*resources);
	}
	return texture;
}

const std::shared_ptr<const Texture>& SpriteSheet::getPaletteTexture() const
{
	Expects(resources != nullptr);
	if (!paletteTexture) {
		loadPaletteTexture(*resources);
	}
	return paletteTexture;
}

const SpriteSheetEntry& SpriteSheet::getSprite(std::string_view name) const
{
	const auto idx = getIndex(name);
	if (!idx) {
		Logger::logError("Spritesheet does not contain sprite \"" + String(name) + "\".");
		return dummySprite;
	}
	return getSprite(idx.value());
}

const SpriteSheetEntry& SpriteSheet::getSprite(size_t idx) const
{
	return sprites[idx];
}

const SpriteSheetEntry* SpriteSheet::tryGetSprite(std::string_view name) const
{
	const auto idx = getIndex(name);
	if (!idx) {
		return {};
	}
	return &getSprite(idx.value());
}

const SpriteSheetEntry& SpriteSheet::getDummySprite() const
{
	return dummySprite;
}

const Vector<SpriteSheetFrameTag>& SpriteSheet::getFrameTags() const
{
	return frameTags;
}

Vector<String> SpriteSheet::getSpriteNames() const
{
	Vector<String> result;
	for (auto& f: spriteIdx) {
		result.push_back(f.first);
	}
	return result;
}

const HashMap<String, uint32_t>& SpriteSheet::getSpriteNameMap() const
{
	return spriteIdx;
}

size_t SpriteSheet::getSpriteCount() const
{
	return sprites.size();
}

std::optional<size_t> SpriteSheet::getIndex(std::string_view name) const
{
	const auto iter = spriteIdx.find(name);
	if (iter == spriteIdx.end()) {
		return {};
	} else {
		return static_cast<size_t>(iter->second);
	}
}

bool SpriteSheet::hasSprite(std::string_view name) const
{
	return spriteIdx.find(name) != spriteIdx.end();
}

const SpriteSheetEntry* SpriteSheet::getSpriteAtTexel(Vector2i pos) const
{
	const auto texSize = getTexture()->getSize();
	const size_t n = getSpriteCount();
	for (size_t i = 0; i < n; ++i) {
		const auto& sprite = getSprite(i);
		auto bounds = sprite.coords * Vector2f(texSize);
		if (bounds.contains(Vector2f(pos))) {
			return &sprite;
		}
	}
	return nullptr;
}

std::unique_ptr<SpriteSheet> SpriteSheet::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<SpriteSheet>();
	result->resources = &loader.getResources();
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	Deserializer s(data->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	result->deserialize(s);

	return result;
}

void SpriteSheet::loadTexture(Resources& resources) const
{
	texture = resources.get<Texture>(textureName);
}

void SpriteSheet::loadPaletteTexture(Resources& resources) const
{
	if (!paletteName.isEmpty()) {
		paletteTexture = resources.get<Texture>(paletteName);
	}
}

void SpriteSheet::assignIds()
{
#ifdef ENABLE_HOT_RELOAD
	for (uint32_t idx = 0; idx < sprites.size(); ++idx) {
		sprites[idx].parent = this;
		sprites[idx].idx = idx;
		sprites[idx].name = "";
	}
	for (const auto& [name, idx]: spriteIdx) {
		sprites[idx].name = name;
	}
	dummySprite.parent = this;
	dummySprite.idx = std::numeric_limits<uint32_t>::max();
	dummySprite.name = "dummy";
#endif	
}

SpriteResource::SpriteResource()
{
}

void SpriteSheet::addSprite(String name, const SpriteSheetEntry& sprite)
{
	sprites.push_back(sprite);
	spriteIdx[std::move(name)] = uint32_t(sprites.size() - 1);
}

void SpriteSheet::setTextureName(String name)
{
	textureName = std::move(name);
}

std::shared_ptr<Material> SpriteSheet::getMaterial(std::string_view name) const
{
	const auto iter = materials.find(name);
	std::shared_ptr<Material> result;
	if (iter != materials.end()) {
		result = iter->second.lock();
	}

	if (!result) {
		result = std::make_shared<Material>(resources->get<MaterialDefinition>(name));
		const auto& textures = result->getDefinition().getTextures();
		for (size_t i = 0; i < textures.size(); ++i) {
			if (textures[i].defaultTextureName == "$palette") {
				result->set(i, getPaletteTexture());
			} else if (i == 0) {
				result->set(i, getTexture());
			}
		}
		materials[name] = result;
	}

	return result;
}

void SpriteSheet::setDefaultMaterialName(String materialName)
{
	defaultMaterialName = std::move(materialName);
}

const String& SpriteSheet::getDefaultMaterialName() const
{
	return defaultMaterialName;
}

void SpriteSheet::setPaletteName(String paletteName)
{
	this->paletteName = paletteName;
}

const String& SpriteSheet::getPaletteName() const
{
	return paletteName;
}

void SpriteSheet::clearMaterialCache() const
{
	materials.clear();
}

void SpriteSheet::reload(Resource&& resource)
{
#ifdef ENABLE_HOT_RELOAD
	auto oldSpriteIdx = spriteIdx;
#endif

	auto& reloaded = dynamic_cast<SpriteSheet&>(resource);

	sprites = std::move(reloaded.sprites);
	spriteIdx = std::move(reloaded.spriteIdx);
	frameTags = std::move(reloaded.frameTags);

	bool updateTextures = false;
	if (textureName != reloaded.textureName) {
		textureName = std::move(reloaded.textureName);
		texture = std::move(reloaded.texture);
		updateTextures = true;
	}
	if (paletteName != reloaded.paletteName) {
		paletteName = std::move(reloaded.paletteName);
		paletteTexture = std::move(reloaded.paletteTexture);
		updateTextures = true;
	}

	if (updateTextures) {
		for (auto& material: materials) {
			if (auto mat = material.second.lock()) {
				const auto& textures = mat->getDefinition().getTextures();
				for (size_t i = 0; i < textures.size(); ++i) {
					if (textures[i].defaultTextureName == "$palette") {
						mat->set(i, getPaletteTexture());
					} else if (i == 0) {
						mat->set(i, getTexture());
					}
				}
			}
		}
	}

	defaultMaterialName = std::move(reloaded.defaultMaterialName);

#ifdef ENABLE_HOT_RELOAD
	// Create idx mapping
	HashMap<uint32_t, uint32_t> idxMap;
	for (const auto& [name, idx]: spriteIdx) {
		const auto iter = oldSpriteIdx.find(name);
		if (iter != oldSpriteIdx.end()) {
			idxMap[iter->second] = idx;
		}
	}

	assignIds();

	// Refresh sprite refs
	for (auto& sprite: spriteRefs) {
		if (sprite.second != std::numeric_limits<uint32_t>::max()) {
			const auto iter = idxMap.find(sprite.second);
			if (iter != idxMap.end()) {
				sprite.second = iter->second;
			} else {
				sprite.second = 0;
			}

			sprite.first->setSprite(sprites.at(sprite.second), sprite.first->hasLastAppliedPivot());
		} else {
			sprite.first->setSprite(dummySprite, sprite.first->hasLastAppliedPivot());
		}
	}
#endif
}

const String& SpriteSheet::getHotReloaderId() const
{
	return getAssetId();
}

void SpriteSheet::serialize(Serializer& s) const
{
	s << version;
	s << textureName;
	s << sprites;
	s << spriteIdx;
	s << frameTags;
	s << defaultMaterialName;
	s << paletteName;
}

void SpriteSheet::deserialize(Deserializer& s)
{
	// Old versions didn't have a version header, so this is a workaround to detect them
	int v;
	s.peek(v);
	if (v <= 255) {
		s >> v;
	} else {
		v = 0;
	}
	
	s >> textureName;
	s >> sprites;
	s >> spriteIdx;
	s >> frameTags;

	if (v >= 1) {
		s >> defaultMaterialName;
	}
	if (v >= 2) {
		s >> paletteName;
	}

	assignIds();
}

void SpriteSheet::onOtherResourcesUnloaded()
{
	if (texture && texture->isUnloaded()) {
		texture = {};
	}
	if (paletteTexture && paletteTexture->isUnloaded()) {
		paletteTexture = {};
	}
}

ResourceMemoryUsage SpriteSheet::getMemoryUsage() const
{
	size_t total = 0;

	total += sprites.capacity() * sizeof(SpriteSheetEntry);
	total += frameTags.capacity() * sizeof(SpriteSheetFrameTag);

	for (const auto& [k, v]: spriteIdx) {
		total += sizeof(String) + k.size() + 16;
	}

	ResourceMemoryUsage result;
	result.ramUsage = total;
	return result;
}

std::unique_ptr<Image> SpriteSheet::generateAtlas(Vector<ImageData>& images, ConfigNode& spriteInfo, bool powerOfTwo)
{
	markDuplicates(images);

	// Generate entries
	int64_t totalImageArea = 0;
	Vector<BinPackEntry> entries;
	entries.reserve(images.size());
	for (auto& img: images) {
		if (!img.isDuplicate) {
			auto size = img.clip.getSize();
			totalImageArea += size.x * size.y;
			entries.emplace_back(size, &img);
		}
	}

	// Figure out a reasonable pack size to start with
	const int minSize = nextPowerOf2(static_cast<int>(sqrt(static_cast<double>(totalImageArea)))) / 2;
	const int64_t guessArea = int64_t(minSize) * int64_t(minSize);
	const int maxSize = 4096;
	int curSize = std::min(maxSize, std::max(32, static_cast<int>(minSize)));

	// Try packing
	bool wide = guessArea > 2 * totalImageArea;
	while (true) {
		Vector2i size(curSize * (wide ? 2 : 1), curSize);
		if (size.x > maxSize || size.y > maxSize) {
			// Give up!
			throw Exception("Unable to pack " + toString(images.size()) + " sprites in a reasonably sized atlas! curSize at " + toString(curSize) + ", maxSize is " + toString(maxSize) + ". Total image area is " + toString(totalImageArea) + " px^2, sqrt = " + toString(lround(sqrt(totalImageArea))) + " px.", HalleyExceptions::Tools);
		}

		//Logger::logInfo("Trying " + toString(size.x) + "x" + toString(size.y) + " px...");
		//auto res = entries.size() < 50 ? BinPack::pack(entries, size) : BinPack::fastPack(entries, size);
		auto res = BinPack::fastPack(entries, size);
		if (res) {
			// Found a pack
			return makeAtlas(res.value(), spriteInfo, powerOfTwo);
		} else {
			// Try 64x64, then 128x64, 128x128, 256x128, etc
			if (wide) {
				wide = false;
				curSize *= 2;
			}
			else {
				wide = true;
			}
		}
	}
}

std::unique_ptr<Image> SpriteSheet::makeAtlas(const Vector<BinPackResult>& result, ConfigNode& spriteInfo, bool powerOfTwo)
{
	spriteInfo.ensureType(ConfigNodeType::Sequence);
	auto& infoSeq = spriteInfo.asSequence();
	
	Vector2i size = computeAtlasSize(result, powerOfTwo);

	std::unique_ptr<Image> atlasImage;

	for (const auto& packedImg: result) {
		const ImageData* img = reinterpret_cast<ImageData*>(packedImg.data);

		if (!atlasImage) {
			atlasImage = std::make_unique<Image>(img->img->getFormat(), size);
			atlasImage->clear(0);
		}
		if (atlasImage->getFormat() != img->img->getFormat()) {
			throw Exception("Mixed image formats in atlas.", HalleyExceptions::Tools);
		}
		
		atlasImage->blitFrom(packedImg.rect.getTopLeft(), *img->img, img->clip, packedImg.rotated);

		const auto borderTL = img->clip.getTopLeft();
		const auto borderBR = img->img->getSize() - img->clip.getSize() - borderTL;

		// This is a 0.1px offset to avoid rounding errors drawing adjacent pixels
		const auto offset = Vector2f(0.1f, 0.1f) / Vector2f(size);

		auto addImageData = [&] (const ImageData& imgData) {
			SpriteSheetEntry entry;
			entry.size = Vector2f(imgData.clip.getSize());
			entry.rotated = packedImg.rotated;
			entry.pivot = imgData.clip.isEmpty() ? Vector2f() : Vector2f(imgData.pivot - imgData.clip.getTopLeft()) / entry.size;
			entry.origPivot = imgData.pivot;
			entry.coords = (Rect4f(Vector2f(packedImg.rect.getTopLeft()) + offset, Vector2f(packedImg.rect.getBottomRight()) - offset)) / Vector2f(size);
			entry.trimBorder = Vector4s(static_cast<short>(borderTL.x), static_cast<short>(borderTL.y), static_cast<short>(borderBR.x), static_cast<short>(borderBR.y));
			entry.slices = imgData.slices;

			for (const auto& filename: imgData.filenames) {
				addSprite(filename, entry);

				auto infoEntry = ConfigNode(ConfigNode::MapType());
				infoEntry["name"] = filename;
				infoEntry["x"] = packedImg.rect.getLeft();
				infoEntry["y"] = packedImg.rect.getTop();
				infoEntry["w"] = packedImg.rect.getWidth();
				infoEntry["h"] = packedImg.rect.getHeight();
				infoEntry["origFilename"] = imgData.origFilename;
				infoEntry["origFrameN"] = imgData.origFrameNumber;
				infoEntry["offX"] = borderTL.x;
				infoEntry["offY"] = borderTL.y;
				infoEntry["rotated"] = packedImg.rotated;
				infoEntry["group"] = imgData.group;
				infoSeq.emplace_back(std::move(infoEntry));
			}
		};
		
		addImageData(*img);
		for (const auto& dupe: img->duplicatesOfThis) {
			addImageData(*dupe);
		}
	}

	return atlasImage;
}

Vector2i SpriteSheet::computeAtlasSize(const Vector<BinPackResult>& results, bool powerOfTwo) const
{
	int w = 0;
	int h = 0;

	for (const auto& r: results) {
		w = std::max(w, r.rect.getRight());
		h = std::max(h, r.rect.getBottom());
	}

	if (powerOfTwo) {
		return Vector2i(nextPowerOf2(w), nextPowerOf2(h));
	} else {
		return Vector2i(w, h);
	}
}

void SpriteSheet::markDuplicates(Vector<ImageData>& images) const
{
	HashMap<uint64_t, ImageData*> hashes;
	
	for (auto& image: images) {
		Hash::Hasher hasher;
		const auto clip = image.clip.intersection(image.img->getRect());
		if (clip.isEmpty()) {
			continue;
		}
		for (int y = clip.getTop(); y < clip.getBottom(); ++y) {
			const auto row = image.img->getPixelBytesRow(clip.getLeft(), clip.getRight(), y);
			hasher.feedBytes(as_bytes(row));
		}
		const auto hash = hasher.digest();

		const auto iter = hashes.find(hash);
		if (iter == hashes.end()) {
			hashes[hash] = &image;
		} else {
			image.isDuplicate = true;
			iter->second->duplicatesOfThis.push_back(&image);
		}
	}
}

bool SpriteSheet::ImageData::operator==(const SpriteSheet::ImageData& other) const
{
	return frameNumber == other.frameNumber
		&& origFrameNumber == other.origFrameNumber
		&& duration == other.duration
		&& sequenceName == other.sequenceName
		&& clip == other.clip
		&& pivot == other.pivot
		&& slices == other.slices
		&& filenames == other.filenames
		&& origFilename == other.origFilename
		&& img->getSize() == other.img->getSize();
}

bool SpriteSheet::ImageData::operator!=(const SpriteSheet::ImageData& other) const
{
	return !(*this == other);
}


SpriteResource::SpriteResource(const std::shared_ptr<const SpriteSheet>& spriteSheet, size_t idx)
	: spriteSheet(spriteSheet)
	, idx(idx)
{
}

SpriteResource::~SpriteResource()
{
#ifdef ENABLE_HOT_RELOAD
	clearSpriteRefs();
#endif
}

const SpriteSheetEntry& SpriteResource::getSprite() const
{
	return getSpriteSheet()->getSprite(idx);
}

size_t SpriteResource::getIdx() const
{
	return idx;
}

std::shared_ptr<const SpriteSheet> SpriteResource::getSpriteSheet() const
{
	return spriteSheet.lock();
}

std::shared_ptr<Material> SpriteResource::getMaterial(std::string_view name) const
{
	return spriteSheet.lock()->getMaterial(name);
}

const String& SpriteResource::getDefaultMaterialName() const
{
	return spriteSheet.lock()->getDefaultMaterialName();
}

std::unique_ptr<SpriteResource> SpriteResource::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<SpriteResource>();
	result->resources = &loader.getResources();
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	Deserializer s(data->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	result->deserialize(s);

	return result;
}

void SpriteResource::reload(Resource&& resource)
{
	auto& reloaded = dynamic_cast<SpriteResource&>(resource);
	spriteSheet = std::move(reloaded.spriteSheet);
	idx = reloaded.idx;

#ifdef ENABLE_HOT_RELOAD
	auto lock = std::unique_lock(spriteMutex);
	for (auto& sprite: spriteRefs) {
		sprite.first->reloadSprite(*this);
	}
#endif
}

const String& SpriteResource::getHotReloaderId() const
{
	return getAssetId();
}

void SpriteResource::serialize(Serializer& s) const
{
	const auto ss = spriteSheet.lock();
	s << ss->getAssetId();
	s << idx;
}

void SpriteResource::deserialize(Deserializer& s)
{
	String ssName;
	s >> ssName;
	spriteSheet = resources->get<SpriteSheet>(ssName);

	s >> idx;
}

ResourceMemoryUsage SpriteResource::getMemoryUsage() const
{
	ResourceMemoryUsage result;
	result.ramUsage = sizeof(SpriteResource);
	return result;
}

#ifdef ENABLE_HOT_RELOAD
void SpriteHotReloader::addSprite(Sprite* sprite, uint32_t idx) const
{
	Expects(sprite != nullptr);
	auto lock = std::unique_lock(spriteMutex);
	spriteRefs[sprite] = idx;
}

void SpriteHotReloader::removeSprite(Sprite* sprite) const
{
	Expects(sprite != nullptr);
	auto lock = std::unique_lock(spriteMutex);
	spriteRefs.erase(sprite);
}

void SpriteHotReloader::updateSpriteIndex(Sprite* sprite, uint32_t idx) const
{
	Expects(sprite != nullptr);
	auto lock = std::unique_lock(spriteMutex);
	spriteRefs.at(sprite) = idx;
}

void SpriteHotReloader::clearSpriteRefs()
{
	auto lock = std::unique_lock(spriteMutex);
	for (auto sprite: spriteRefs) {
		sprite.first->clearSpriteSheetRef();
	}
}

std::size_t SpriteHotReloader::SpritePointerHasher::operator()(Sprite* ptr) const noexcept
{
	return reinterpret_cast<size_t>(ptr) / sizeof(Sprite);
}
#endif
