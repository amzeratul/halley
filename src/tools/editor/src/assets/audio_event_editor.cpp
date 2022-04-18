#include "audio_event_editor.h"

#include "src/ui/select_asset_widget.h"
using namespace Halley;

AudioEventEditor::AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioEvent)
{
	factory.loadUI(*this, "halley/audio_editor/audio_event_editor");
}

void AudioEventEditor::reload()
{
	doLoadUI();
}

void AudioEventEditor::refreshAssets()
{
	// TODO
}

void AudioEventEditor::onMakeUI()
{
	actionList = getWidgetAs<UIList>("actions");
	doLoadUI();
}

Resources& AudioEventEditor::getGameResources() const
{
	return gameResources;
}

void AudioEventEditor::update(Time t, bool moved)
{
	// TODO
}

std::shared_ptr<const Resource> AudioEventEditor::loadResource(const String& id)
{
	audioEvent = std::make_shared<AudioEvent>(*gameResources.get<AudioEvent>(id));
	return audioEvent;
}

void AudioEventEditor::doLoadUI()
{
	if (audioEvent) {
		getWidgetAs<UILabel>("title")->setText(LocalisedString::fromHardcodedString("Audio Event: \"" + audioEvent->getAssetId() + "\""));
		actionList->clear();

		for (auto& action : audioEvent->getActions()) {
			auto a = std::make_shared<AudioEventEditorAction>(factory, *this, *action, actionId++);
			auto id = a->getId();
			actionList->addItem(id, std::move(a));
		}
	}
}

AudioEventEditorAction::AudioEventEditorAction(UIFactory& factory, AudioEventEditor& editor, IAudioEventAction& action, int id)
	: UIWidget(toString(id), {}, UISizer())
	, factory(factory)
	, editor(editor)
	, action(action)
{
	factory.loadUI(*this, "halley/audio_editor/audio_action");
}

void AudioEventEditorAction::onMakeUI()
{
	switch (action.getType()) {
	case AudioEventActionType::Play:
		makePlayAction(dynamic_cast<AudioEventActionPlay&>(action));
		break;
	case AudioEventActionType::Stop:
		makeStopAction(dynamic_cast<AudioEventActionStop&>(action));
		break;
	case AudioEventActionType::Pause:
		makePauseAction(dynamic_cast<AudioEventActionPause&>(action));
		break;
	case AudioEventActionType::Resume:
		makeResumeAction(dynamic_cast<AudioEventActionResume&>(action));
		break;
	}
}

void AudioEventEditorAction::makeObjectAction(AudioEventActionObject& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_play");

	auto updateFadeType = [this](AudioFadeCurve curve)
	{
		getWidget("fadeLenOptions")->setActive(curve != AudioFadeCurve::None);
	};
	
	bindData("object", action.getObjectName(), [=, &action] (String value)
	{
		action.setObjectName(value, editor.getGameResources());
	});
	
	bindData("fadeType", toString(action.getFade().getCurve()), [=, &action] (String value)
	{
		auto curve = fromString<AudioFadeCurve>(value);
		action.getFade().setCurve(curve);
		updateFadeType(curve);
	});
	updateFadeType(action.getFade().getCurve());

	bindData("fadeLength", action.getFade().getLength(), [=, &action] (float value)
	{
		action.getFade().setLength(value);
	});	
}

void AudioEventEditorAction::makePlayAction(AudioEventActionPlay& action)
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString("Play"));

	makeObjectAction(action);

	getWidget("playOptions")->setActive(true);

	bindData("delay", action.getDelay(), [=, &action] (float value)
	{
		action.setDelay(value);
	});
	
	bindData("gainMin", action.getGain().start, [=, &action] (float value)
	{
		action.getGain().start = value;
	});	
	
	bindData("gainMax", action.getGain().end, [=, &action] (float value)
	{
		action.getGain().end = value;
	});	
}

void AudioEventEditorAction::makeStopAction(AudioEventActionStop& action)
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString("Stop"));

	makeObjectAction(action);
}

void AudioEventEditorAction::makePauseAction(AudioEventActionPause& action)
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString("Pause"));

	makeObjectAction(action);
}

void AudioEventEditorAction::makeResumeAction(AudioEventActionResume& action)
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString("Resume"));

	makeObjectAction(action);	
}
