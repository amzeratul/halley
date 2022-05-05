#include "audio_sequence_editor.h"

#include "audio_object_editor.h"
using namespace Halley;

AudioSequenceEditor::AudioSequenceEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSequence& sequence)
	: UIWidget("audio_sequence_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, sequence(sequence)
{
	factory.loadUI(*this, "halley/audio_editor/audio_sequence_editor");
}

void AudioSequenceEditor::onMakeUI()
{
	// TODO
}

void AudioSequenceEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioObjectEditor& AudioSequenceEditor::getEditor()
{
	return editor;
}
