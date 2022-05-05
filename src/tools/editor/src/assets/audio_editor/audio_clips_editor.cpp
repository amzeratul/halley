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
	getWidget("loopDetails")->setActive(clips.getLoop());

	bindData("loop", clips.getLoop(), [=] (bool value)
	{
		clips.setLoop(value);
		editor.markModified(false);
		getWidget("loopDetails")->setActive(value);
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

	bindData("loopStart", clips.getLoopStart(), [=] (int value)
	{
		clips.setLoopStart(value);
		editor.markModified(false);
	});

	bindData("loopEnd", clips.getLoopEnd(), [=] (int value)
	{
		clips.setLoopEnd(value);
		editor.markModified(false);
	});
}
