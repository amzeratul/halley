#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include "halley/file_formats/json/json.h"
#include <halley/file_formats/json_file.h>
#include "halley/file/byte_serializer.h"

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
	s << pivot;
	s << size;
	s << coords;
	s << duration;
	s << rotated;
}

void SpriteSheetEntry::deserialize(Deserializer& s)
{
	s >> pivot;
	s >> size;
	s >> coords;
	s >> duration;
	s >> rotated;
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

size_t SpriteSheet::getSpriteCount() const
{
	return sprites.size();
}

size_t SpriteSheet::getIndex(const String& name) const
{
	auto iter = spriteIdx.find(name);
	if (iter == spriteIdx.end()) {
		throw Exception("Spritesheet does not contain sprite \"" + name + "\".");
	} else {
		return size_t(iter->second);
	}
}

std::unique_ptr<SpriteSheet> SpriteSheet::loadResource(ResourceLoader& loader)
{
	auto result = std::make_unique<SpriteSheet>();
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	result->deserialize(s);
	result->loadTexture(loader.getAPI().core->getResources());

	return std::move(result);
}

void SpriteSheet::loadTexture(Resources& resources)
{
	texture = resources.get<Texture>(textureName);
}

void SpriteSheet::addSprite(String name, const SpriteSheetEntry& sprite)
{
	sprites.push_back(sprite);
	spriteIdx[name] = uint32_t(sprites.size() - 1);
}

void SpriteSheet::setTextureName(String name)
{
	textureName = name;
}

void SpriteSheet::serialize(Serializer& s) const
{
	s << textureName;
	s << sprites;
	s << spriteIdx;
	s << frameTags;
}

void SpriteSheet::deserialize(Deserializer& s)
{
	s >> textureName;
	s >> sprites;
	s >> spriteIdx;
	s >> frameTags;
}

void SpriteSheet::loadJson(gsl::span<const gsl::byte> data)
{
	auto src = reinterpret_cast<const char*>(data.data());

	// Parse json
	Json::Reader reader;
	JSONValue root;
	reader.parse(src, src + data.size(), root);

	// Read Metadata
	auto meta = root["meta"];
	Vector2f scale = Vector2f(1, 1);
	textureName = "";
	if (meta) {
		if (meta["image"]) {
			textureName = meta["image"].asString();
			Vector2f textureSize = readSize<Vector2f>(meta["size"]);
			scale = Vector2f(1.0f / textureSize.x, 1.0f / textureSize.y);
		}
		if (meta["frameTags"]) {
			for (auto& frameTag: meta["frameTags"]) {
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
		Vector2f pivot = readVector<Vector2f>(sprite["pivot"]);
		Vector2i pivotPos = Vector2i(int(pivot.x * sourceSize.x + 0.5f), int(pivot.y * sourceSize.y + 0.5f));
		Vector2i newPivotPos = pivotPos - spriteSourceSize.getTopLeft();
		entry.pivot = Vector2f(newPivotPos) / Vector2f(spriteSourceSize.getSize());

		if (sprite["duration"]) {
			entry.duration = sprite["duration"].asInt();
		}
		
		addSprite(iter.memberName(), entry);
	}	
}
