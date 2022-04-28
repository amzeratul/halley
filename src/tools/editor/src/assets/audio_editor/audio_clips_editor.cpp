#include "audio_clips_editor.h"

#include "audio_object_editor.h"
using namespace Halley;

AudioClipsEditor::AudioClipsEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectClips& clips)
	: UIWidget("audio_clips_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, clips(clips)
{
	factory.loadUI(*this, "halley/audio_editor/audio_clips_editor");
}

void AudioClipsEditor::onMakeUI()
{
	bindData("loop", clips.getLoop(), [=] (bool value)
	{
		clips.setLoop(value);
		editor.markModified(false);
	});

	bindData("gainMin", clips.getGain().start, [=] (float value)
	{
		clips.getGain().start = value;
		editor.markModified(false);
	});

	bindData("gainMax", clips.getGain().end, [=] (float value)
	{
		clips.getGain().end = value;
		editor.markModified(false);
	});
}
