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
#ifdef DEV_BUILD
	for (auto sprite: spriteRefs) {
		sprite.first->clearSpriteSheetRef();
	}
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
	return getSprite(getIndex(name));
}

const SpriteSheetEntry& SpriteSheet::getSprite(size_t idx) const
{
	return sprites[idx];
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

size_t SpriteSheet::getIndex(const String& name) const
{
	auto iter = spriteIdx.find(name);
	if (iter == spriteIdx.end()) {
		String names = "";
		bool first = true;
		for (auto& f: spriteIdx) {
			if (first) {
				first = false;
				names += "\"";
			} else {
				names += "\", \"";
			}
			names += f.first;
		}
		if (!spriteIdx.empty()) {
			names += "\"";
		}
		throw Exception("Spritesheet does not contain sprite \"" + name + "\".\nSprites: { " + names + " }.", HalleyExceptions::Resources);
	} else {
		return size_t(iter->second);
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
	auto data = loader.getStatic();
	Deserializer s(data->getSpan(), SerializerOptions(SerializerOptions::maxVersion));
	result->deserialize(s);

	return result;
}

void SpriteSheet::loadTexture(Resources& resources) const
{
	texture = resources.get<Texture>(textureName);
}

#ifdef DEV_BUILD
void SpriteSheet::addSprite(Sprite* sprite, uint32_t idx) const
{
	spriteRefs[sprite] = idx;
}

void SpriteSheet::removeSprite(Sprite* sprite) const
{
	spriteRefs.erase(sprite);
}
#endif

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
		result->set("tex0", getTexture());
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
#ifdef DEV_BUILD
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
				mat->set("tex0", texture);
			}
		}
	}

	defaultMaterialName = std::move(reloaded.defaultMaterialName);

#ifdef DEV_BUILD
	// Create idx mapping
	HashMap<uint32_t, uint32_t> idxMap;
	for (const auto& [name, idx]: spriteIdx) {
		const auto iter = oldSpriteIdx.find(name);
		if (iter != oldSpriteIdx.end()) {
			idxMap[iter->second] = idx;
		}
	}

	// Assign indices to all sprites
	for (uint32_t idx = 0; idx < sprites.size(); ++idx) {
		sprites[idx].parent = this;
		sprites[idx].idx = idx;
	}

	// Refresh sprite refs
	for (auto& sprite: spriteRefs) {
		const auto iter = idxMap.find(sprite.second);
		if (iter != idxMap.end()) {
			sprite.second = iter->second;
		} else {
			sprite.second = 0;
		}
		
		sprite.first->setSprite(sprites[sprite.second], sprite.first->hasLastAppliedPivot());
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

	for (uint32_t idx = 0; idx < sprites.size(); ++idx) {
		sprites[idx].parent = this;
		sprites[idx].idx = idx;
	}
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
	*this = std::move(dynamic_cast<SpriteResource&>(resource));
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
