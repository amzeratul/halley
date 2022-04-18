#include "audio_event_editor.h"

#include "halley/tools/project/project.h"
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
	if (audioEvent) {
		audioEvent = std::make_shared<AudioEvent>(*gameResources.get<AudioEvent>(audioEvent->getAssetId()));
		doLoadUI();
	}
}

void AudioEventEditor::onMakeUI()
{
	actionList = getWidgetAs<UIList>("actions");

	setHandle(UIEventType::ListItemsSwapped, "actions", [=](const UIEvent& event)
	{
		auto& actions = audioEvent->getActions();
		std::swap(actions[event.getIntData()], actions[event.getIntData2()]);
		markModified();
	});

	setHandle(UIEventType::ButtonClicked, "addAction", [=] (const UIEvent& event)
	{
		addAction();
	});

	doLoadUI();
}

void AudioEventEditor::save()
{
	if (modified) {
		modified = false;

		const auto assetPath = Path("audio_event/" + audioEvent->getAssetId() + ".yaml");
		const auto strData = audioEvent->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);
	}
}

bool AudioEventEditor::isModified()
{
	return modified;
}

void AudioEventEditor::markModified()
{
	modified = true;
}

void AudioEventEditor::addAction()
{
	getRoot()->addChild(std::make_shared<ChooseAudioEventAction>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			addAction(fromString<AudioEventActionType>(*result));
		}
	}));
}

void AudioEventEditor::addAction(AudioEventActionType type)
{
	auto& actions = audioEvent->getActions();
	actions.emplace_back(AudioEvent::makeAction(type));
	addActionUI(*actions.back());
	markModified();
}

void AudioEventEditor::deleteAction(const IAudioEventAction& action, const String& uiId)
{
	Concurrent::execute(Executors::getMainUpdateThread(), [this, &action, uiId=uiId]() {
		actionList->removeItem(uiId);
		auto& actions = audioEvent->getActions();
		std_ex::erase_if(actions, [&](const auto& a) { return a.get() == &action; });
		markModified();
	});
}

Resources& AudioEventEditor::getGameResources() const
{
	return gameResources;
}

void AudioEventEditor::update(Time t, bool moved)
{
}

std::shared_ptr<const Resource> AudioEventEditor::loadResource(const String& id)
{
	audioEvent = std::make_shared<AudioEvent>(*gameResources.get<AudioEvent>(id));
	return audioEvent;
}

void AudioEventEditor::addActionUI(IAudioEventAction& action)
{
	auto a = std::make_shared<AudioEventEditorAction>(factory, *this, action, actionId++);
	auto id = a->getId();
	actionList->addItem(id, std::move(a), 1);
}

void AudioEventEditor::doLoadUI()
{
	if (audioEvent) {
		getWidgetAs<UILabel>("title")->setText(LocalisedString::fromHardcodedString("Audio Event: \"" + audioEvent->getAssetId() + "\""));
		actionList->clear();

		for (auto& action : audioEvent->getActions()) {
			addActionUI(*action);
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
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromUserString(AudioEvent::getActionName(action.getType())));

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
	case AudioEventActionType::SetVolume:
		makeSetVolumeAction(dynamic_cast<AudioEventActionSetVolume&>(action));
		break;
	case AudioEventActionType::SetSwitch:
		makeSetSwitchAction(dynamic_cast<AudioEventActionSetSwitch&>(action));
		break;
	case AudioEventActionType::SetVariable:
		makeSetVariableAction(dynamic_cast<AudioEventActionSetVariable&>(action));
		break;
	}

	setHandle(UIEventType::ButtonClicked, "delete", [=] (const UIEvent& event)
	{
		editor.deleteAction(action, getId());
	});
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
		editor.markModified();
	});
	
	bindData("fadeType", toString(action.getFade().getCurve()), [=, &action] (String value)
	{
		auto curve = fromString<AudioFadeCurve>(value);
		action.getFade().setCurve(curve);
		editor.markModified();
		updateFadeType(curve);
	});
	updateFadeType(action.getFade().getCurve());

	bindData("fadeLength", action.getFade().getLength(), [=, &action] (float value)
	{
		action.getFade().setLength(value);
		editor.markModified();
	});	
}

void AudioEventEditorAction::makePlayAction(AudioEventActionPlay& action)
{
	makeObjectAction(action);

	getWidget("playOptions")->setActive(true);
	getWidgetAs<SelectAssetWidget>("object")->setAllowEmpty({});

	bindData("delay", action.getDelay(), [=, &action] (float value)
	{
		action.setDelay(value);
		editor.markModified();
	});
	
	bindData("gainMin", action.getGain().start, [=, &action] (float value)
	{
		action.getGain().start = value;
		editor.markModified();
	});	
	
	bindData("gainMax", action.getGain().end, [=, &action] (float value)
	{
		action.getGain().end = value;
		editor.markModified();
	});	
}

void AudioEventEditorAction::makeStopAction(AudioEventActionStop& action)
{
	makeObjectAction(action);
}

void AudioEventEditorAction::makePauseAction(AudioEventActionPause& action)
{
	makeObjectAction(action);
}

void AudioEventEditorAction::makeResumeAction(AudioEventActionResume& action)
{
	makeObjectAction(action);	
}

void AudioEventEditorAction::makeSetVolumeAction(AudioEventActionSetVolume& action)
{
	makeObjectAction(action);

	getWidget("volumeOptions")->setActive(true);
	
	bindData("gain", action.getGain(), [=, &action] (float value)
	{
		action.setGain(value);
		editor.markModified();
	});	
}

void AudioEventEditorAction::makeSetSwitchAction(AudioEventActionSetSwitch& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_set_variable");

	getWidget("switchValue")->setActive(true);

	bindData("variableId", action.getSwitchId(), [=, &action](String value)
	{
		action.setSwitchId(std::move(value));
		editor.markModified();
	});
	
	bindData("switchValue", action.getValue(), [=, &action](String value)
	{
		action.setValue(std::move(value));
		editor.markModified();
	});
}

void AudioEventEditorAction::makeSetVariableAction(AudioEventActionSetVariable& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_set_variable");

	getWidget("variableValue")->setActive(true);

	bindData("variableId", action.getVariableId(), [=, &action](String value)
	{
		action.setVariableId(std::move(value));
		editor.markModified();
	});
	
	bindData("variableValue", action.getValue(), [=, &action](float value)
	{
		action.setValue(value);
		editor.markModified();
	});	
}

ChooseAudioEventAction::ChooseAudioEventAction(UIFactory& factory, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
{
	Vector<String> ids;
	Vector<String> names;
	for (auto id: EnumNames<AudioEventActionType>()()) {
		ids.push_back(id);
		names.push_back(AudioEvent::getActionName(fromString<AudioEventActionType>(id)));
	}
	setTitle(LocalisedString::fromHardcodedString("Add Audio Event Action"));
	setAssetIds(ids, names, "play");
}

void ChooseAudioEventAction::sortItems(Vector<std::pair<String, String>>& items)
{
}
