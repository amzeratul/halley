#include "graphics/sprite/sprite_sheet.h"
#include "halley/graphics/texture.h"
#include "halley/api/halley_api.h"
#include "resources/resources.h"
#include "../../../../utils/src/file_formats/json/json.h"

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

std::unique_ptr<SpriteSheet> SpriteSheet::loadResource(ResourceLoader& loader)
{
	// Read data
	auto data = loader.getStatic();
	auto src = static_cast<const char*>(data->getData());

	// Parse json
	Json::Reader reader;
	JSONValue root;
	reader.parse(src, src + data->getSize(), root);

	// Create sprite sheet
	auto result = std::make_unique<SpriteSheet>();
	
	// Read Metadata
	auto meta = root["meta"];
	String textureName = meta["image"].asString();
	Vector2f textureSize = readSize<Vector2f>(meta["size"]);
	Vector2f scale = Vector2f(1.0f / textureSize.x, 1.0f / textureSize.y);
	result->texture = loader.getAPI().getResource<Texture>(textureName);

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
		entry.coords = Rect4f(frame.getP1() * scale, texSize.x * scale.x, texSize.y * scale.y);
		entry.size = size;

		const Vector2i sourceSize = readSize<Vector2i>(sprite["sourceSize"]);
		const Rect4i spriteSourceSize = readRect<Rect4i>(sprite["spriteSourceSize"]);
		Vector2f pivot = readVector<Vector2f>(sprite["pivot"]);
		Vector2i pivotPos = Vector2i(int(pivot.x * sourceSize.x + 0.5f), int(pivot.y * sourceSize.y + 0.5f));
		Vector2i newPivotPos = pivotPos - spriteSourceSize.getP1();
		entry.pivot = Vector2f(newPivotPos) / Vector2f(spriteSourceSize.getSize());
		
		result->sprites[iter.memberName()] = entry;
	}

	return std::move(result);
}
