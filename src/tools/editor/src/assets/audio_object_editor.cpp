#include "audio_object_editor.h"
#include "halley/audio/audio_object.h"
using namespace Halley;

AudioObjectEditor::AudioObjectEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::AudioObject)
{
	
}

void AudioObjectEditor::refresh()
{
	// TODO
}

void AudioObjectEditor::reload()
{
	// TODO
}

void AudioObjectEditor::refreshAssets()
{
	// TODO
}

void AudioObjectEditor::update(Time t, bool moved)
{
	// TODO
}

std::shared_ptr<const Resource> AudioObjectEditor::loadResource(const String& assetId)
{
	return gameResources.get<AudioObject>(assetId);
}
