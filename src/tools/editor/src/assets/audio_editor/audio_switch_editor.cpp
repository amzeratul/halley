#include "audio_switch_editor.h"

using namespace Halley;

AudioSwitchEditor::AudioSwitchEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSwitch& switchConfig)
	: UIWidget("audio_switch_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, switchConfig(switchConfig)
{
	factory.loadUI(*this, "halley/audio_editor/audio_switch_editor");
}
