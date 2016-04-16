#include "sprite_sheet.h"
#include "../texture.h"
#include "../../api/halley_api.h"
#include "../../resources/resources.h"
#include "../../../../utils/src/file_formats/json/json.h"

using namespace Halley;

static Rect4i readRect4i(JSONValue value)
{
	return Rect4i(value["x"], value["y"], value["w"], value["h"]);
}

static Rect4f readRect4f(JSONValue value)
{
	return Rect4f(value["x"], value["y"], value["w"], value["h"]);
}

static Vector2f readVector2f(JSONValue value)
{
	return Vector2f(value["x"], value["y"]);
}

static Vector2f readSizeVector2f(JSONValue value)
{
	return Vector2f(value["w"], value["h"]);
}

std::unique_ptr<SpriteSheet> SpriteSheet::loadResource(ResourceLoader& loader)
{
	// Find base path
	String basePath = loader.getName();
	size_t lastSlash = basePath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		basePath = basePath.left(lastSlash + 1);
	}

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
	String textureName = basePath + meta["image"].asString();
	Vector2f textureSize = readSizeVector2f(meta["size"]);
	Vector2f scale = Vector2f(1.0f / textureSize.x, 1.0f / textureSize.y);
	result->texture = loader.getAPI().core->getResources().get<Texture>(textureName);

	// Read sprites
	auto frames = root["frames"];
	for (auto iter = frames.begin(); iter != frames.end(); ++iter) {
		auto sprite = *iter;

		SpriteSheetEntry entry;
		entry.rotated = sprite["rotated"].asBool();
		
		Rect4f frame = readRect4f(sprite["frame"]);
		Vector2f size = frame.getSize();
		if (entry.rotated) {
			std::swap(size.x, size.y);
		}
		entry.coords = Rect4f(frame.getP1() * scale, size.x * scale.x, size.y * scale.y);
		entry.size = size;

		Rect4i spriteSourceSize = readRect4i(sprite["spriteSourceSize"]);
		Vector2f pivot = readVector2f(sprite["pivot"]);
		entry.pivot = pivot;
		
		result->sprites[iter.memberName()] = entry;
	}

	return std::move(result);
}
