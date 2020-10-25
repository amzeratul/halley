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
#include "halley/audio/audio_event.h"
#include "halley/file_formats/binary_file.h"
#include "halley/file_formats/image.h"
#include "halley/core/graphics/mesh/mesh.h"
#include "halley/utils/variable.h"

using namespace Halley;

void StandardResources::initialize(Resources& resources)
{
	resources.init<Animation>();
	resources.init<SpriteSheet>();
	resources.init<SpriteResource>();
	resources.init<Texture>();
	resources.init<Image>();
	resources.init<MaterialDefinition>();
	resources.init<ShaderFile>();
	resources.init<BinaryFile>();
	resources.init<TextFile>();
	resources.init<Font>();
	resources.init<ConfigFile>();
	resources.init<AudioClip>();
	resources.init<AudioEvent>();
	resources.init<Mesh>();
	resources.init<VariableTable>();
	resources.init<Prefab>();
	resources.init<Scene>();

	resources.setFallback<Animation>("missing_image");
	resources.setFallback<SpriteSheet>("missing_image");
	resources.setFallback<SpriteResource>("missing_image");
	resources.setFallback<Texture>("missing_image");

	resources.setFallback<MaterialDefinition>("Halley/Sprite");

	resources.setFallback<Font>("Ubuntu Bold");

	resources.setFallback<AudioEvent>("missing_audio_event");

	resources.setFallback<Prefab>("missing_prefab");
	resources.setFallback<Scene>("missing_scene");
}
