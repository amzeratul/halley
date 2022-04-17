#include "audio_event_editor.h"
using namespace Halley;

AudioEventEditor::AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioEvent)
{
	factory.loadUI(*this, "halley/audio_event_editor");
}

void AudioEventEditor::reload()
{
	// TODO
}

void AudioEventEditor::refreshAssets()
{
	// TODO
}

void AudioEventEditor::update(Time t, bool moved)
{
	// TODO
}

std::shared_ptr<const Resource> AudioEventEditor::loadResource(const String& assetId)
{
	audioEvent = std::make_shared<AudioEvent>(*gameResources.get<AudioEvent>(assetId));
	return audioEvent;
}
