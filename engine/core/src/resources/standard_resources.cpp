#include <halley/file_formats/text_file.h>
#include "resources/standard_resources.h"
#include "resources/resources.h"
#include "graphics/sprite/animation.h"
#include "graphics/sprite/sprite_sheet.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/material/material_definition.h"
#include "graphics/text/font.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

void StandardResources::initialize(Resources& resources)
{
	resources.init<Animation>("animation");
	resources.init<SpriteSheet>("spritesheet");
	resources.init<Texture>("image");
	resources.init<MaterialDefinition>("material");
	resources.init<TextFile>("");
	resources.init<Font>("font");
	resources.init<ConfigFile>("config");
}
