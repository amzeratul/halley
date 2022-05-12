#include "audio_sequence_editor.h"

#include "audio_fade_editor.h"
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
	getWidget("fadeContainer")->add(std::make_shared<AudioFadeEditor>(factory, sequence.getCrossFade(), [=] ()
	{
		editor.markModified(false);
	}));

	bindData("sequenceType", toString(sequence.getSequenceType()), [=] (String value)
	{
		sequence.getSequenceType() = fromString<AudioSequenceType>(value);
		editor.markModified(false);
	});
}

void AudioSequenceEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioObjectEditor& AudioSequenceEditor::getEditor()
{
	return editor;
}
