#include <halley/file_formats/text_file.h>
#include "resources/standard_resources.h"
#include "resources/resources.h"
#include "graphics/sprite/animation.h"
#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/material/material_definition.h"
#include "graphics/text/font.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/file_formats/binary_file.h"

using namespace Halley;

void StandardResources::initialize(Resources& resources)
{
	resources.init<Animation>();
	resources.init<SpriteSheet>();
	resources.init<SpriteResource>();
	resources.init<Texture>();
	resources.init<MaterialDefinition>();
	resources.init<ShaderFile>();
	resources.init<BinaryFile>();
	resources.init<TextFile>();
	resources.init<Font>();
	resources.init<ConfigFile>();
	resources.init<AudioClip>();

	resources.of<SpriteResource>().setResourceLoader([&] (const String& name, ResourceLoadPriority) -> std::shared_ptr<Resource>
	{
		auto& sprites = resources.of<SpriteResource>();

		std::shared_ptr<Resource> result;

		auto& ss = resources.of<SpriteSheet>();
		for (auto& sheetName: ss.enumerate()) {
			auto sheet = ss.get(sheetName);
			for (auto& spriteName: sheet->getSpriteNames()) {
				if (spriteName.startsWith(":img:")) {
					auto res = std::make_shared<SpriteResource>(sheet, sheet->getIndex(spriteName));
					sprites.setResource(0, spriteName, res);

					if (spriteName == ":img:" + name) {
						result = res;
					}
				}
			}
		}

		return result;
	});
}
