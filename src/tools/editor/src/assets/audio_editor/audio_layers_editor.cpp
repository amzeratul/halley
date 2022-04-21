#include "audio_layers_editor.h"

using namespace Halley;

AudioLayersEditor::AudioLayersEditor(UIFactory& factory, AudioSubObjectLayers& layers)
	: UIWidget("audio_layers_editor", Vector2f(), UISizer())
	, factory(factory)
	, layers(layers)
{
	factory.loadUI(*this, "halley/audio_editor/audio_layers_editor");
}
