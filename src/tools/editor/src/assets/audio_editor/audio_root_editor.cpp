#include "audio_root_editor.h"
using namespace Halley;

AudioRootEditor::AudioRootEditor(UIFactory& factory, AudioObject& object)
	: UIWidget("audio_root_editor", Vector2f(), UISizer())
	, factory(factory)
	, object(object)
{
	factory.loadUI(*this, "halley/audio_editor/audio_root_editor");
}
