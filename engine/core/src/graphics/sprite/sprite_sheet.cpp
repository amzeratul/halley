#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "resources/resources.h"
#include "halley/file_formats/json/json.h"
#include <halley/file_formats/json_file.h>

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

const SpriteSheetEntry& SpriteSheet::getSprite(String name) const
{
	auto iter = sprites.find(name);
	if (iter == sprites.end()) {
		throw Exception("Spritesheet does not contain sprite \"" + name + "\".");
	} else {
		return iter->second;
	}
}

const std::vector<SpriteSheetFrameTag>& SpriteSheet::getFrameTags() const
{
	return frameTags;
}

std::vector<String> SpriteSheet::getSpriteNames() const
{
	std::vector<String> result;
	for (auto& f: sprites) {
		result.push_back(f.first);
	}
	return result;
}

std::unique_ptr<SpriteSheet> SpriteSheet::loadResource(ResourceLoader& loader)
{
	// Create sprite sheet
	auto result = std::make_unique<SpriteSheet>();
	auto data = loader.getStatic();
	result->loadJson(data->getSpan());

	// Load texture
	result->texture = loader.getAPI().getResource<Texture>(result->textureName);

	return std::move(result);
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
		
		sprites[iter.memberName()] = entry;
	}
	
}
