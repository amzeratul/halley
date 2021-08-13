#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include "halley/file_formats/json/json.h"
#include <halley/file_formats/json_file.h>
#include "graphics/material/material.h"
#include "graphics/material/material_definition.h"
#include "graphics/sprite/sprite.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/logger.h"

using namespace Halley;

template <typename T>
static T readRect(JSONValue value)
{
	return T(value["x"], value["y"], value["w"], value["h"]);
}

template <typename T>
static T readVector(JSONValue value)
{
	return T(value["x"], value["y"]);
}

template <typename T>
static T readSize(JSONValue value)
{
	return T(value["w"], value["h"]);
}

void SpriteSheetEntry::serialize(Serializer& s) const
{
	Expects(pivot.isValid());
	Expects(size.isValid());
	
	s << pivot;
	s << origPivot;
	s << size;
	s << coords;
	s << duration;
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
	s >> duration;
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
	: defaultMaterialName("Halley/Sprite")
{
}

SpriteSheet::~SpriteSheet()
{
#ifdef ENABLE_HOT_RELOAD
	clearSpriteRefs();
#endif
}

const std::shared_ptr<const Texture>& SpriteSheet::getTexture() const
{
	Expects(resources != nullptr);
	if (!texture) {
		loadTexture(*resources);
	}
	return texture;
}

const SpriteSheetEntry& SpriteSheet::getSprite(const String& name) const
{
	const auto idx = getIndex(name);
	if (!idx) {
		Logger::logError("Spritesheet does not contain sprite \"" + name + "\".");
		return dummySprite;
	}
	return getSprite(idx.value());
}

const SpriteSheetEntry& SpriteSheet::getSprite(size_t idx) const
{
	return sprites[idx];
}

const SpriteSheetEntry* SpriteSheet::tryGetSprite(const String& name) const
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

const std::vector<SpriteSheetFrameTag>& SpriteSheet::getFrameTags() const
{
	return frameTags;
}

std::vector<String> SpriteSheet::getSpriteNames() const
{
	std::vector<String> result;
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

std::optional<size_t> SpriteSheet::getIndex(const String& name) const
{
	const auto iter = spriteIdx.find(name);
	if (iter == spriteIdx.end()) {
		return {};
	} else {
		return static_cast<size_t>(iter->second);
	}
}

bool SpriteSheet::hasSprite(const String& name) const
{
	return spriteIdx.find(name) != spriteIdx.end();
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

void SpriteSheet::assignIds()
{
#ifdef ENABLE_HOT_RELOAD
	for (uint32_t idx = 0; idx < sprites.size(); ++idx) {
		sprites[idx].parent = this;
		sprites[idx].idx = idx;
	}
	dummySprite.parent = this;
	dummySprite.idx = std::numeric_limits<uint32_t>::max();
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

std::shared_ptr<Material> SpriteSheet::getMaterial(const String& name) const
{
	const auto iter = materials.find(name);
	std::shared_ptr<Material> result;
	if (iter != materials.end()) {
		result = iter->second.lock();
	}

	if (!result) {
		result = std::make_shared<Material>(resources->get<MaterialDefinition>(name));
		if (!result->getDefinition().getTextures().empty()) {
			result->set(0, getTexture());
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

	if (textureName != reloaded.textureName) {
		textureName = std::move(reloaded.textureName);
		texture = std::move(reloaded.texture);

		for (auto& material: materials) {
			auto mat = material.second.lock();
			if (mat) {
				mat->set(0, texture);
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

void SpriteSheet::serialize(Serializer& s) const
{
	s << version;
	s << textureName;
	s << sprites;
	s << spriteIdx;
	s << frameTags;
	s << defaultMaterialName;
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

	assignIds();
}

void SpriteSheet::loadJson(gsl::span<const gsl::byte> data)
{
	auto src = reinterpret_cast<const char*>(data.data());

	// Parse json
	Json::Reader reader;
	JSONValue root;
	reader.parse(src, src + data.size(), root);

	// Read Metadata
	auto metadataNode = root["meta"];
	Vector2f scale = Vector2f(1, 1);
	textureName = "";
	if (metadataNode) {
		if (metadataNode["image"]) {
			textureName = metadataNode["image"].asString();
			Vector2f textureSize = readSize<Vector2f>(metadataNode["size"]);
			scale = Vector2f(1.0f / textureSize.x, 1.0f / textureSize.y);
		}
		if (metadataNode["frameTags"]) {
			for (auto& frameTag: metadataNode["frameTags"]) {
				frameTags.push_back(SpriteSheetFrameTag());
				auto& f = frameTags.back();
				f.name = frameTag["name"].asString();
				f.from = frameTag["from"].asInt();
				f.to = frameTag["to"].asInt();
			}
		}
	}

	// Read sprites
	auto frames = root["frames"];
	for (auto iter = frames.begin(); iter != frames.end(); ++iter) {
		auto sprite = *iter;

		SpriteSheetEntry entry;
		entry.rotated = sprite["rotated"].asBool();
		
		Rect4f frame = readRect<Rect4f>(sprite["frame"]);
		Vector2f size = frame.getSize();
		Vector2f texSize = size;
		if (entry.rotated) {
			std::swap(texSize.x, texSize.y);
		}
		entry.coords = Rect4f(frame.getTopLeft() * scale, texSize.x * scale.x, texSize.y * scale.y);
		entry.size = size;

		const Vector2i sourceSize = readSize<Vector2i>(sprite["sourceSize"]);
		const Rect4i spriteSourceSize = readRect<Rect4i>(sprite["spriteSourceSize"]);
		Vector2f rawPivot = readVector<Vector2f>(sprite["pivot"]);
		Vector2i pivotPos = Vector2i(int(rawPivot.x * sourceSize.x + 0.5f), int(rawPivot.y * sourceSize.y + 0.5f));
		Vector2i newPivotPos = pivotPos - spriteSourceSize.getTopLeft();
		entry.pivot = Vector2f(newPivotPos) / Vector2f(spriteSourceSize.getSize());

		if (sprite["duration"]) {
			entry.duration = sprite["duration"].asInt();
		}
		
		addSprite(iter.memberName(), entry);
	}	
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

std::shared_ptr<Material> SpriteResource::getMaterial(const String& name) const
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
	for (auto& sprite: spriteRefs) {
		sprite.first->reloadSprite(*this);
	}
#endif
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

#ifdef ENABLE_HOT_RELOAD
void SpriteHotReloader::addSprite(Sprite* sprite, uint32_t idx) const
{
	Expects(sprite != nullptr);
	spriteRefs[sprite] = idx;
}

void SpriteHotReloader::removeSprite(Sprite* sprite) const
{
	Expects(sprite != nullptr);
	spriteRefs.erase(sprite);
}

void SpriteHotReloader::updateSpriteIndex(Sprite* sprite, uint32_t idx) const
{
	Expects(sprite != nullptr);
	spriteRefs.at(sprite) = idx;
}

void SpriteHotReloader::clearSpriteRefs()
{
	for (auto sprite: spriteRefs) {
		sprite.first->clearSpriteSheetRef();
	}
}

std::size_t SpriteHotReloader::SpritePointerHasher::operator()(Sprite* ptr) const noexcept
{
	return reinterpret_cast<size_t>(ptr) / sizeof(Sprite);
}
#endif
