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
	}
}

void AudioEventEditorAction::makePlayAction(AudioEventActionPlay& action)
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromHardcodedString("Play"));

	factory.loadUI(*getWidget("contents"), "halley/audio_editor/audio_action_play");
	
	bindData("object", action.getObjectName(), [=, &action] (String value)
	{
		action.setObjectName(value, editor.getGameResources());
	});
}
