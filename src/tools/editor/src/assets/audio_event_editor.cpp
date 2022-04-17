#include "audio_event_editor.h"
using namespace Halley;

AudioEventEditor::AudioEventEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioEvent)
{
	
}

void AudioEventEditor::refresh()
{
	// TODO
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
	return gameResources.get<AudioEvent>(assetId);
}
