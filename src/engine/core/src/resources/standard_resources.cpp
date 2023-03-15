#include <halley/file_formats/text_file.h>
#include "halley/resources/standard_resources.h"

#include "halley/properties/game_properties.h"
#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/resources/resources.h"
#include "halley/graphics/sprite/animation.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/graphics/shader.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/text/font.h"
#include "halley/file_formats/config_file.h"
#include "halley/audio/audio_clip.h"
#include "halley/audio/audio_event.h"
#include "halley/audio/audio_object.h"
#include "halley/file_formats/binary_file.h"
#include "halley/file_formats/image.h"
#include "halley/graphics/mesh/mesh.h"
#include "halley/entity/prefab.h"
#include "halley/scripting/script_graph.h"
#include "halley/navigation/navmesh_set.h"
#include "halley/ui/ui_definition.h"
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
	resources.init<AudioObject>();
	resources.init<AudioEvent>();
	resources.init<Mesh>();
	resources.init<VariableTable>();
	resources.init<Prefab>();
	resources.init<Scene>();
	resources.init<NavmeshSet>();
	resources.init<RenderGraphDefinition>();
	resources.init<ScriptGraph>();
	resources.init<UIDefinition>();
	resources.init<GameProperties>();

	resources.setFallback<Animation>("missing_image");
	resources.setFallback<SpriteSheet>("missing_image");
	resources.setFallback<SpriteResource>("missing_image");
	resources.setFallback<Texture>("missing_image.ase");

	resources.setFallback<MaterialDefinition>(MaterialDefinition::defaultMaterial);

	resources.setFallback<Font>("Ubuntu Bold");

	resources.setFallback<ConfigFile>("missing_config");
	
	resources.setFallback<AudioEvent>("missing_audio_event");

	resources.setFallback<Prefab>("missing_prefab");
	resources.setFallback<Scene>("missing_scene");

	resources.setFallback<GameProperties>("default_properties");
}
