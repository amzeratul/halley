#include "audio_event_editor.h"

#include "audio_fade_editor.h"
#include "halley/properties/game_properties.h"
#include "halley/tools/project/project.h"
#include "src/ui/select_asset_widget.h"
using namespace Halley;

AudioEventEditor::AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioEvent)
{
	factory.loadUI(*this, "halley/audio_editor/audio_event_editor");
}

void AudioEventEditor::onResourceLoaded()
{
	doLoadUI();
}

void AudioEventEditor::refreshAssets()
{
	if (audioEvent) {
		audioEvent = std::make_shared<AudioEvent>(YAMLConvert::parseConfig(project.getAssetsSrcPath() / assetPath).getRoot());
		audioEvent->setAssetId(assetId);
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

void AudioEventEditor::deleteAction(const AudioEventAction& action, const String& uiId)
{
	Concurrent::execute(Executors::getMainUpdateThread(), [this, &action, uiId=uiId]() {
		actionList->removeItem(uiId);
		auto& actions = audioEvent->getActions();
		std_ex::erase_if(actions, [&](const auto& a) { return a.get() == &action; });
		markModified();
	});
}

const AudioProperties& AudioEventEditor::getAudioProperties() const
{
	return project.getGameProperties().getAudioProperties();
}

Resources& AudioEventEditor::getGameResources() const
{
	return gameResources;
}

std::shared_ptr<const Resource> AudioEventEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);

	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		audioEvent = std::make_shared<AudioEvent>(config.getRoot());
	} else {
		audioEvent = std::make_shared<AudioEvent>();
		markModified();
	}
	audioEvent->setAssetId(assetId);
	
	return audioEvent;
}

void AudioEventEditor::addActionUI(AudioEventAction& action)
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

AudioEventEditorAction::AudioEventEditorAction(UIFactory& factory, AudioEventEditor& editor, AudioEventAction& action, int id)
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
	case AudioEventActionType::CopySwitch:
		makeCopySwitchAction(dynamic_cast<AudioEventActionCopySwitch&>(action));
		break;
	case AudioEventActionType::SetVariable:
		makeSetVariableAction(dynamic_cast<AudioEventActionSetVariable&>(action));
		break;
	case AudioEventActionType::PauseBus:
	case AudioEventActionType::StopBus:
		makeBusAction(dynamic_cast<AudioEventActionBus&>(action));
		break;
	case AudioEventActionType::ResumeBus:
		makeResumeBusAction(dynamic_cast<AudioEventActionResumeBus&>(action));
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

	getWidget("fadeContainer")->add(std::make_shared<AudioFadeEditor>(factory, action.getFade(), [=] ()
	{
		editor.markModified();
	}));

	bindData("object", action.getObjectName(), [=, &action] (String value)
	{
		action.setObjectName(value, editor.getGameResources());
		editor.markModified();
	});

	bindData("scope", toString(action.getScope()), [=, &action](String value)
	{
		action.setScope(fromString<AudioEventScope>(value));
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
	
	bindData("pitchMin", action.getPitch().start, [=, &action] (float value)
	{
		action.getPitch().start = value;
		editor.markModified();
	});	
	
	bindData("pitchMax", action.getPitch().end, [=, &action] (float value)
	{
		action.getPitch().end = value;
		editor.markModified();
	});	
	
	bindData("singleton", action.isSingleton(), [=, &action] (bool value)
	{
		action.setSingleton(value);
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

	getWidget("resumeOptions")->setActive(true);

	bindData("force", action.getForce(), [=, &action] (bool value)
	{
		action.setForce(value);
		editor.markModified();
	});	
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

	getWidgetAs<UIDropdown>("variableId")->setOptions(editor.getAudioProperties().getSwitchIds());

	getWidget("switchValue")->setActive(true);

	auto populateSwitchValues = [=] (const String& id)
	{
		const auto* switchProps = editor.getAudioProperties().tryGetSwitch(id);
		auto dropdown = getWidgetAs<UIDropdown>("switchValue");
		if (switchProps) {
			dropdown->setOptions(switchProps->getValues());
		}  else {
			dropdown->clear();
		}
	};

	bindData("variableId", action.getSwitchId(), [=, &action](String value)
	{
		populateSwitchValues(value);
		action.setSwitchId(std::move(value));
		editor.markModified();
	});
	populateSwitchValues(action.getSwitchId());
	
	bindData("switchValue", action.getValue(), [=, &action](String value)
	{
		action.setValue(std::move(value));
		editor.markModified();
	});

	bindData("scope", toString(action.getScope()), [=, &action](String value)
	{
		action.setScope(fromString<AudioEventScope>(value));
		editor.markModified();
	});
}

void AudioEventEditorAction::makeCopySwitchAction(AudioEventActionCopySwitch& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_copy_switch");

	getWidgetAs<UIDropdown>("dstId")->setOptions(editor.getAudioProperties().getSwitchIds());
	getWidgetAs<UIDropdown>("srcId")->setOptions(editor.getAudioProperties().getSwitchIds());

	bindData("dstId", action.getDstSwitchId(), [=, &action](String value)
	{
		action.setDstSwitchId(std::move(value));
		editor.markModified();
	});

	bindData("srcId", action.getSrcSwitchId(), [=, &action](String value)
	{
		action.setSrcSwitchId(std::move(value));
		editor.markModified();
	});

	bindData("scope", toString(action.getScope()), [=, &action](String value)
	{
		action.setScope(fromString<AudioEventScope>(value));
		editor.markModified();
	});
}

void AudioEventEditorAction::makeSetVariableAction(AudioEventActionSetVariable& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_set_variable");

	getWidgetAs<UIDropdown>("variableId")->setOptions(editor.getAudioProperties().getVariableIds());

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

	bindData("scope", toString(action.getScope()), [=, &action](String value)
	{
		action.setScope(fromString<AudioEventScope>(value));
		editor.markModified();
	});
}

void AudioEventEditorAction::makeBusAction(AudioEventActionBus& action)
{
	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_bus");

	getWidgetAs<UIDropdown>("busName")->setOptions(editor.getAudioProperties().getBusIds());

	getWidget("fadeContainer")->add(std::make_shared<AudioFadeEditor>(factory, action.getFade(), [=] ()
	{
		editor.markModified();
	}));

	bindData("busName", action.getBusName(), [=, &action](String value)
	{
		action.setBusName(std::move(value));
		editor.markModified();
	});
}

void AudioEventEditorAction::makeResumeBusAction(AudioEventActionResumeBus& action)
{
	makeBusAction(action);

	getWidget("resumeOptions")->setActive(true);

	bindData("force", action.getForce(), [=, &action] (bool value)
	{
		action.setForce(value);
		editor.markModified();
	});	
}

ChooseAudioEventAction::ChooseAudioEventAction(UIFactory& factory, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
{
	Vector<String> ids;
	Vector<String> names;
	for (auto id: EnumNames<AudioEventActionType>()()) {
		const auto type = fromString<AudioEventActionType>(id);
		ids.push_back(id);
		names.push_back(AudioEvent::getActionName(type));
	}
	setTitle(LocalisedString::fromHardcodedString("Add Audio Event Action"));
	setAssetIds(ids, names, "play");
}

void ChooseAudioEventAction::sortItems(Vector<std::pair<String, String>>& items)
{
}
