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
		actionList->clear();

		for (auto& action : audioEvent->getActions()) {
			auto a = std::make_shared<AudioEventEditorAction>(factory, *action, actionId++);
			auto id = a->getId();
			actionList->addItem(id, std::move(a));
		}
	}
}

AudioEventEditorAction::AudioEventEditorAction(UIFactory& factory, IAudioEventAction& action, int id)
	: UIWidget(toString(id), {}, UISizer())
	, factory(factory)
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

	getWidgetAs<SelectAssetWidget>("object")->setValue(action.getObjectName());
}
